#include "ic_display.h"


IC_DISPLAY::IC_DISPLAY(CANBUS_COMMUNICATOR *can) {
    canB = can;
}

uint8_t IC_DISPLAY::current_page = IC_PAGE_AUDIO;

bool IC_DISPLAY::can_fit_body_text(const char *text) {
    int length = 0;
    for (uint8_t i = 0; i < strlen(text); i++) {
        length += pgm_read_byte_near(CHAR_WIDTHS + text[i]);
    }
    return (length - 1) <= DISPLAY_WIDTH_PX; // -1 as last char doesn't need 1 pixel padding
}

uint8_t IC_DISPLAY::getChecksum(uint8_t len, uint8_t* payload) {
    uint8_t cs = 0xFF;
    for (uint8_t i = 0; i <= len; i++) cs -= i + payload[i];
    return cs;
}

bool IC_DISPLAY::setHeader(uint8_t p, const char* text, uint8_t fmt) {
    if (strlen(text) == 0) return false;

    DPRINTLN(F("-- Update header --"));
    uint8_t str_len = min(strlen(text), 20U);
    buffer_size = str_len + 3;
    buffer[0] = p; // Page number
    buffer[1] = 0x29; // Package 29 (Header text update)
    buffer[2] = fmt; // Text justification
    for (uint8_t i = 0; i < str_len; i++) {
        buffer[i+3] = text[i];
    }
    buffer[buffer_size] = 0x00;
    buffer[buffer_size+1] = getChecksum(buffer_size, buffer);
    buffer_size+=2;
    if (!sendBytes(400,0)) {
        return false;
    }
    if (fc.header.isFlag && fc.header.page == p) {
        fc.header.isFlag = false;
        if (millis() - fc.header.timestamp < 0 + FC_TIMEOUT_MS) {
            return fc.header.ack;
        }
    }
    return false;
}

bool IC_DISPLAY::setBody(uint8_t p, const char* text, uint8_t fmt = IC_TEXT_FMT_CENTER_JUSTIFICATION) {
    if (strlen(text) == 0) return false;

    DPRINTLN(F("-- Update body --"));
    uint8_t str_len = min(strlen(text), 32U);
    buffer_size = str_len + 7; // Not including CS bit
    buffer[0] = p; // Page number
    buffer[1] = 0x26; // Package 26 (Body text update)
    buffer[2] = 0x01; 
    buffer[3] = 0x00;
    buffer[4] = 0x01; // Number of strings (1 only)
    buffer[5] = str_len + 2;
    buffer[6] = fmt;
    for (uint8_t i = 0; i < str_len; i++) {
        buffer[i+7] = text[i];
    }
    buffer[buffer_size] = 0x00; // End of string
    buffer[buffer_size+1] = getChecksum(buffer_size, buffer);
    buffer_size+=2;
    if (!sendBytes(400,0)) {
        return false;
    }
    if (fc.body.isFlag && fc.body.page == p) {
        fc.body.isFlag = false;
        if (millis() - fc.body.timestamp < 0 + FC_TIMEOUT_MS) {
            return fc.body.ack;
        }
    }
    return false;
}

bool IC_DISPLAY::setBodyTel(uint8_t numStrs, const char* lines[]){
    uint8_t charCount = 0;
    uint8_t strsToUse = 0;
    for (uint8_t i = 0; i < numStrs; i++) {
        charCount += strlen(lines[i]) + 2;
        if (charCount < 55) {
            strsToUse++;
        }
    }
    if (strsToUse == 0) {
        return false;
    }
    DPRINTLN(F("-- Update body --"));
    buffer_size = charCount + 5; // Not including CS bit
    buffer[0] = IC_PAGE_TELEPHONE; // Page number
    buffer[1] = 0x26; // Package 26 (Body text update)
    buffer[2] = 0x01; 
    buffer[3] = 0x00;
    buffer[4] = strsToUse; // Number of strings
    uint8_t index = 5;
    for (uint8_t i = 0; i < strsToUse; i++) {
        buffer[index] = strlen(lines[i]) + 2;
        buffer[index+1] = IC_TEXT_FMT_CENTER_JUSTIFICATION;
        index += 2;
        for (uint8_t x = 0; x < strlen(lines[i]); x++) {
            buffer[index] = lines[i][x];
            index++;
        }
    }
    buffer[buffer_size] = 0x00; // End of string
    buffer[buffer_size+1] = getChecksum(buffer_size, buffer);
    buffer_size+=2;
    if (!sendBytes(400,0)) {
        return false;
    }
    if (fc.body.isFlag && fc.body.page == IC_PAGE_TELEPHONE) {
        fc.body.isFlag = false;
        if (millis() - fc.body.timestamp < 0 + FC_TIMEOUT_MS) {
            return fc.body.ack;
        }
    }
    return false;
}

void IC_DISPLAY::processIcResponse(can_frame *r) {
    static unsigned long estimated = millis();
    if (estimated < millis()) {
        estimated = millis();
        fc.busy = false;
    }
    if (r->can_id == RECEIVE_CAN_ID) {
        DPRINTLN(IC_TO_AGW_STR+*canB->frame_to_string(r, false));
        // Parse Flow Control Frames
        if ((r->data[0] & 0xF0) == 0x30) {
            fc.state = r->data[0];
            fc.bs = r->data[1];
            fc.stMin = r->data[2];
            fc.timestamp = millis();
            return;
        }
        // Parse Single-Frame ACKs
        if (r->data[0] == 0x03) {
            uint8_t page = r->data[1];
            uint8_t pkg = r->data[2];
            bool isTrue = (r->data[3] == 0x06);
            ack_t *frame = nullptr;
            if (page == 0x03 || page == 0x05) {
                if (pkg == 0x24) frame = &fc.initPage;
                else if (pkg == 0x26) frame = &fc.body;
                else if (pkg == 0x29) frame = &fc.header;
                if (frame) {
                    frame->isFlag = true;
                    frame->ack = isTrue;
                    frame->page = page;
                    frame->timestamp = millis();
                }
                return;
            } else {
                frame = &fc.other;
                frame->isFlag = true;
                frame->ack = isTrue;
                frame->timestamp = millis();
            }
        }
        // Some data relating to navigation sent to AGW
        if (r->data[0] == 0x06 && r->data[2] == 0x27) {
            // Audio Page
            if (r->data[1] == 0x03 && r->data[6] == 0xC4) { // Move in
                current_page = IC_PAGE_AUDIO;
            }
            else if (r->data[1] == 0x03 && r->data[6] == 0xC3) { // Move out
                current_page = IC_PAGE_OTHER;
            }
            // Telephone page
            if (r->data[1] == 0x05 && r->data[6] == 0xC2) { // Move in
                current_page = IC_PAGE_TELEPHONE;
            }
            else if (r->data[1] == 0x05 && r->data[6] == 0xC1) { // Move out
                current_page = IC_PAGE_OTHER;
            }
            return;
        }
    } else if (r->can_id == SEND_CAN_ID) {
        DPRINTLN(AGW_TO_IC_STR+*canB->frame_to_string(r, false));
        // Parse ISO-TP First Frame (FF)
        if ((r->data[0] & 0xF0) == 0x10) {
            uint16_t len = ((r->data[0] & 0x0F) << 8) | r->data[1];
            uint16_t remaining = (len > 6) ? (len - 6) : 0;
            uint16_t count = (remaining + 6) / 7;
            estimated += (unsigned long)count * (fc.stMin < 0xF0 ? fc.stMin : POLL_STEP_MS);
		}
        fc.busy = true;
        return;
    }
}


bool IC_DISPLAY::initPage(uint8_t p, const char* header, uint8_t fmt, uint8_t upper_Symbol, uint8_t lower_Symbol, uint8_t numLines=1) {
    DPRINTLN(F("-- Init page --"));
    uint8_t str_len = min(strlen(header), 20U);
    buffer_size = str_len + 17; // Not including CS bit
    buffer[0] = p; // Page number
    buffer[1] = 0x24; // Package 24 (Init page)
    buffer[2] = 0x02;
    buffer[3] = 0x60;
    buffer[4] = 0x00;
    buffer[5] = numLines;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x00;
    buffer[9] = 0x13;
    buffer[10] = upper_Symbol; // Upper symbol (Above text)
    buffer[11] = 0x01;
    buffer[12] = lower_Symbol; // Lower symbol (Under text)
    buffer[13] = 0x02;
    buffer[14] = 0x00;
    buffer[15] = str_len;   
    buffer[16] = fmt;
    for (uint8_t i = 0; i < str_len; i++) {
        buffer[i+17] = header[i];
    }
    buffer[buffer_size] = 0x00; // End of string
    buffer[buffer_size+1] = getChecksum(buffer_size, buffer);
    buffer_size+=2;
    if (!sendBytes(400,400)) {
        return false;
    }
    if (fc.initPage.isFlag && fc.initPage.page == p) {
        fc.initPage.isFlag = false;
        if (millis() - fc.initPage.timestamp < 400 + FC_TIMEOUT_MS) {
            return fc.initPage.ack;
        }
    }
    return false;
}


bool IC_DISPLAY::sendBytes(int pre_delay, int post_delay) {
    x.can_id = SEND_CAN_ID;
    x.can_dlc = 0x08;
    if (buffer_size == 0) {
        return false;
    } else if (buffer_size >= 55) {
        Serial.println(F("Payload too big!"));
        return false;
    }
    // helper for STmin delay
    auto st_min = [this]() {
        if (fc.stMin > 0) {
            if (fc.stMin < 0xF0) {
                wait(fc.stMin);
            } else {
                wait(POLL_STEP_MS); // 0xF9 < 1 ms
            }
        } else {
            wait(40); // fallback
        }
    };
    // Step 0: wait for silence on SEND_CAN_ID
    unsigned long preStart = millis();
    while (fc.busy && millis() - preStart < (unsigned long)pre_delay + POLL_STEP_MS) {
        wait(POLL_STEP_MS);
    }
    // Step 1: wait for any ACK Frame
    while (!receivedACK() && millis() - preStart < (unsigned long)pre_delay + POLL_STEP_MS) {
        wait(POLL_STEP_MS);
    }
    if (fc.busy) {
        DPRINTLN(F("ACK timeout!"));
        return false;
    }
    if (buffer_size < 8) {
        // Step 2: send Single Frame (SF)
        x.data[0] = buffer_size;
        for (uint8_t i = 0; i < buffer_size; i++) {
            x.data[i + 1] = buffer[i];
        }
        DPRINTLN(AGW_TO_IC_STR + *canB->frame_to_string(&x, false));
        canB->sendToBus(&x);
        wait(post_delay);
    } else {
        // Step 2: send First Frame (FF)
        x.data[0] = 0x10;
        x.data[1] = buffer_size;
        for (uint8_t i = 2; i < 8; i++) {
            x.data[i] = buffer[i - 2];
        }
        DPRINTLN(AGW_TO_IC_STR + *canB->frame_to_string(&x, false));
        canB->sendToBus(&x);
        // Step 3: wait for Flow Control Frame (FC)
        unsigned long timeout = millis();
        while (!receivedFC() && millis() - timeout < FC_TIMEOUT_MS) {
            wait(POLL_STEP_MS);
        }
        if (!receivedFC()) {
            DPRINTLN(F("FC timeout!"));
            return false;
        }
        // Step 4: read BS and STmin, then send Consecutive Frames (CF)
        if (fc.state == 0x30) {
            unsigned long lastFc = fc.timestamp;
            uint8_t frameCount = fc.bs;
            uint8_t framesSent = 0;
            uint8_t seqId = 1;
            for (int offset = 6; offset < buffer_size; offset += 7) {
//                st_min(); // wait(40);
                wait(2); // speed up faster than allowed
                if (fc.busy || fc.timestamp != lastFc) {
                    // another AGW >> IC transfer detected, abort
                    return false;
                }
                x.data[0] = 0x20 | (seqId & 0x0F);
                for (uint8_t i = 1; i < 8; i++) {
                    int bufIdx = offset + (i - 1);
                    if (bufIdx < buffer_size) {
                        x.data[i] = buffer[bufIdx];
                    } else {
                        x.data[i] = 0x00;
                    }
                }
                DPRINTLN(AGW_TO_IC_STR + *canB->frame_to_string(&x, false));
                canB->sendToBus(&x);
                seqId++;
                framesSent++;
                // if block size limit is reached, wait for next FC update
                if (frameCount > 0 && framesSent >= frameCount) {
                    timeout = millis();
                    while (!receivedFC() && millis() - timeout < FC_TIMEOUT_MS) {
                        wait(POLL_STEP_MS);
                    }
                    if (!receivedFC()) {
                        DPRINTLN(F("FC timeout!"));
                        return false;
                    }
                    if (fc.state == 0x30) {
                        lastFc = fc.timestamp;
                        frameCount = fc.bs;
                        framesSent = 0;
                    }
                }
            }
            wait(post_delay + FC_TIMEOUT_MS); // poll for ACK
        } else {
            return false;
        }
    }
    return true;
}


void IC_DISPLAY::wait(unsigned long msec) {
    unsigned long endTime = millis() + msec;
    canB->clear_frame();
    while (millis() <= endTime) {
        processIcResponse(canB->read_frame());
    }
}

bool IC_DISPLAY::receivedFC() {
    return (millis() - fc.timestamp < FC_TIMEOUT_MS) && ((fc.state & 0xF0) == 0x30);
}

bool IC_DISPLAY::receivedACK() {
    unsigned long now = millis();
    bool ret = (fc.other.isFlag    && now - fc.other.timestamp    < FC_TIMEOUT_MS)
            || (fc.initPage.isFlag && now - fc.initPage.timestamp < FC_TIMEOUT_MS)
            || (fc.body.isFlag     && now - fc.body.timestamp     < FC_TIMEOUT_MS)
            || (fc.header.isFlag   && now - fc.header.timestamp   < FC_TIMEOUT_MS);

    fc.other.isFlag    = false;
    fc.initPage.isFlag = false;
    fc.body.isFlag     = false;
    fc.header.isFlag   = false;
    return ret;
}


bool IC_DISPLAY::update() {
    const unsigned long now = millis();
    uint8_t ret = 0x3F;
    if (replayPageEnabled) {
        if (now - replayPageStartedAt > PAGE_REPLAY_LIFETIME_MS) {
            replayPageEnabled = false;
        } else if (int(now - replayPageLastAt) > (int)replayPageRepeat + adHocRetryPage) {
            if (!initPage(
                IC_PAGE_TELEPHONE,
                pendingPageText,
                IC_TEXT_FMT_CENTER_JUSTIFICATION,
                IC_SYMB_NONE,
                IC_SYMB_NONE,
                replayBodyLineCount
            )) ret &= ~0x01;
            if (!setHeader(
                IC_PAGE_TELEPHONE,
                pendingPageText,
                IC_TEXT_FMT_CENTER_JUSTIFICATION
            )) ret &= ~0x02;
            if (ret & 0x01) {
                replayPageEnabled = false;
            }
            replayPageLastAt = now;
            replayBodyLastAt = now;
            adHocRetryPage = 0;
            adHocRetryBody = 0;
        }
    }
    if (replayBodyEnabled && int(now - replayBodyLastAt) > (int)replayBodyRepeat + adHocRetryBody) {
        if (now - replayBodyStartedAt < BODY_REPLAY_LIFETIME_MS) {
            const char* lines[MAX_BODY_LINES] = {
                replayBody[0],
                replayBody[1],
                replayBody[2],
                replayBody[3]
            };
            if (!setBodyTel(replayBodyLineCount, lines)) {
                ret &= ~0x08;
            }
            if (!setHeader(
                    IC_PAGE_TELEPHONE,
                    replayPageText,
                    IC_TEXT_FMT_CENTER_JUSTIFICATION)) {
                ret &= ~0x04;
            }
        } else {
            const char* lines[MAX_BODY_LINES] = {
                "AIRmatic",
                replayBody[1],
                "OK",
                ""
            };
            if (!setBodyTel(replayBodyLineCount, lines)) {
                ret &= ~0x20;
            }
            if (!setHeader(
                    IC_PAGE_TELEPHONE,
                    pendingPageText,
                    IC_TEXT_FMT_CENTER_JUSTIFICATION)) {
                ret &= ~0x10;
            }
            if ((ret & 0x28) == 0x28) {
                replayBodyEnabled = false;
            } else {
                replayBodyStartedAt += replayBodyRepeat;
            }
        }
        replayBodyLastAt = now;
        adHocRetryBody = 0;
    }
    // retry on fail: fire up again immediately
    if (!(ret & 0x01)) {
        adHocRetryPage = RETRY_ON_FAIL_INTERVAL_MS - (int)replayPageRepeat;
    }
    else if ((ret & 0x28) != 0x28) {
        adHocRetryBody = RETRY_ON_FAIL_INTERVAL_MS - (int)replayBodyRepeat;
    }
    return (ret == 0x3F);
}

void IC_DISPLAY::update(const char* pageText, unsigned long repeat) {
    if (pageText == nullptr || repeat == 0) {
        return;
    }
    snprintf(pendingPageText, sizeof(pendingPageText), "%s", pageText);
    replayPageRepeat = repeat;
    replayPageEnabled = true;
    replayPageStartedAt = millis();
}

void IC_DISPLAY::update(const char* pageText, uint8_t bodyLineCount, const char* const* bodyLines, unsigned long repeat) {
    if (pageText == nullptr ||
        bodyLines == nullptr ||
        bodyLineCount == 0 ||
        bodyLineCount > MAX_BODY_LINES ||
        repeat == 0) {
        return;
    }
    snprintf(replayPageText, sizeof(replayPageText), "%s", pageText);
    for (uint8_t i = 0; i < bodyLineCount; i++) {
        snprintf(
            replayBody[i],
            sizeof(replayBody[i]),
            "%s",
            bodyLines[i] != nullptr ? bodyLines[i] : ""
        );
    }
    replayBodyLineCount = bodyLineCount;
    replayBodyRepeat = repeat;
    replayBodyEnabled = true;
    replayBodyStartedAt = millis();
}
