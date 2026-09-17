void canEvent0(void *pvParameters) {
  bool rxbox0 = false;
  bool rxbox1 = false;
  while (true) {
    if (canInterruptFlag0) {
      portENTER_CRITICAL(&mux_can0);
      canInterruptFlag0 = false;
      portEXIT_CRITICAL(&mux_can0);
      if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        uint8_t irq = Can0->getInterrupts();
        if (irq & MCP2515::CANINTF_RX0IF) {
          if (Can0->readMessage(MCP2515::RXB0, &canMsg[0]) == MCP2515::ERROR_OK) {
            rxbox0 = true;
          }
        }
        if (irq & MCP2515::CANINTF_RX1IF) {
          if (Can0->readMessage(MCP2515::RXB1, &canMsg[1]) == MCP2515::ERROR_OK) {
            rxbox1 = true;
          }
        }
        Can0->clearInterrupts();
        xSemaphoreGive(spiMutex);
      }
      if (rxbox0) {
        // export CAN_message into bit field decoder
        exportMsg(canMsg[0].can_id, canMsg[0].data, canMsg[0].can_dlc);
        rxbox0 = false;
      }
      if (rxbox1) {
        // export CAN_message into bit field decoder
        exportMsg(canMsg[1].can_id, canMsg[1].data, canMsg[1].can_dlc);
        rxbox1 = false;
      }
      awake(100); // prevent idle timeout
    }
    // send mailbox queue with delay 100 ms
    sendMbx(Can0, &mbox0, 100);
    delay_us(1000);
  }
}

void canEvent1(void *pvParameters) {
  bool rxbox0 = false;
  bool rxbox1 = false;
  while (true) {
    if (canInterruptFlag1) {
      portENTER_CRITICAL(&mux_can1);
      canInterruptFlag1 = false;
      portEXIT_CRITICAL(&mux_can1);
      if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        uint8_t irq = Can1->getInterrupts();
        if (irq & MCP2515::CANINTF_RX0IF) {
          if (Can1->readMessage(MCP2515::RXB0, &canMsg[2]) == MCP2515::ERROR_OK) {
            rxbox0 = true;
          }
        }
        if (irq & MCP2515::CANINTF_RX1IF) {
          if (Can1->readMessage(MCP2515::RXB1, &canMsg[3]) == MCP2515::ERROR_OK) {
            rxbox1 = true;
          }
        }
        Can1->clearInterrupts();
        xSemaphoreGive(spiMutex);
      }
      if (rxbox0) {
        // export CAN_message into bit field decoder
        exportMsg(canMsg[2].can_id, canMsg[2].data, canMsg[2].can_dlc);
        rxbox0 = false;
      }
      if (rxbox1) {
        // export CAN_message into bit field decoder
        exportMsg(canMsg[3].can_id, canMsg[3].data, canMsg[3].can_dlc);
        rxbox1 = false;
      }
      awake(100); // prevent idle timeout
    }
    // send mailbox queue with delay 100 ms
    sendMbx(Can1, &mbox1, 100);
    delay_us(1000);
  }
}

bool enqueueFrame(mbx_t *mbox, const struct can_frame *msg) {
  if (mbox->count < MAILBOX_CAPACITY) {
    mbox->frames[mbox->tail] = *msg;
    mbox->tail = (mbox->tail + 1) % MAILBOX_CAPACITY;
    mbox->count++;
    return true;
  } else {
    return false;
  }
}

bool dequeueFrame(mbx_t *mbox, struct can_frame *msg) {
  if (mbox->count) {
    *msg = mbox->frames[mbox->head];
    mbox->head = (mbox->head + 1) % MAILBOX_CAPACITY;
    mbox->count--;
    return true;
  } else {
    return false;
  }
}

void sendMsg(mbx_t *mbox, canid_t id, const void *msg, size_t len, uint8_t repeat) {
  struct can_frame buf;
  buf.can_id  = id;
  buf.can_dlc = len > CAN_MAX_DLEN ? CAN_MAX_DLEN : len;
  memcpy(buf.data, msg, buf.can_dlc);
  for (uint8_t i = 0; i < repeat; i++) {
    if (!enqueueFrame(mbox, &buf)) {
      Serial.printf(
        "Message not sent: CanId 0x%04X: send mailbox capacity exceeded!\n",
        buf.can_id
      );
      delay(100);
    }
  }
}

void sendMbx(MCP2515 *can, mbx_t *mbox, unsigned long min_interval_ms) {
  unsigned long now = millis();
  if (mbox->count) {
    if (now - mbox->timestamp > min_interval_ms) {
      struct can_frame msg;
      if (dequeueFrame(mbox, &msg)) {
        mbox->timestamp = now;
        if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE) {
          can->sendMessage(&msg);
          xSemaphoreGive(spiMutex);
        }

        // debug output
        Serial.printf("0x%04X: (%02d):", msg.can_id, msg.can_dlc);
        for (uint8_t i = 0; i < msg.can_dlc; i++) {
          Serial.printf(" %02X", msg.data[i]);
        }
        Serial.println();

      }
    }
  }
}