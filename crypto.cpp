/*                                                                          *
 * Mbed TLS Knowledge Base                                                  *
 *                                                                          *
 * RSA 2048-bit decryption in C with Mbed TLS                               *
 *                                                                          *
 * mbed-tls.readthedocs.io/en/latest/kb/how-to/encrypt-and-decrypt-with-rsa *
 *                                                                          *
 *                                                                          */
#include "crypto.h"

uint8_t enc_ssid[32];
uint8_t enc_pass[32];
uint8_t hw_key[16];
uint8_t fw_key[256];
uint8_t enc_seed[256];
uint8_t pseudoKey[32];
uint8_t challenge[33];
size_t fwKeyLen;
size_t seedLen;

bool rsaKeys = false;


// read MAC from eFuse
esp_err_t getMac(uint8_t* key) {
  uint64_t mac;
  esp_err_t ret = esp_efuse_mac_get_default((uint8_t*)&mac);
  if (ret != ESP_OK) {
    return ret;
  }
  for (int i = 0; i < 16; i++) {
    uint8_t byte = ((uint8_t*)&mac)[i % 6] ^ (0xA5 + i);
    if (byte == 0x00) {
      byte = (uint8_t)(i + 1);
    }
    key[i] = byte;
  }
  return ret;
}


// compare two arrays
bool handshake(const uint8_t *a, size_t size_a, const uint8_t *b, size_t size_b) {
  size_t limit = size_a < size_b ? size_a : size_b;
  for (size_t i = 0; i < limit; ++i) {
    if (a[i] == '\0' && b[i] == '\0') {
      return true;
    }
    if (a[i] != b[i]) {
      return false;
    }
  }
  if (size_a > size_b) {
    return a[size_b] == '\0';
  }
  if (size_a < size_b) {
    return b[size_a] == '\0';
  }
  return true;
}


// convert bytes -> ASCII hex string
String bytesToHex(const uint8_t *data, size_t len) {

  // strip from end and make len a multiplier of 16
  while (len-- > 0 && data[len] == 0x00) {
    continue;
  }
  len = (++len + 15) & ~15;

  String hexStr;
  hexStr.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 16) hexStr += '0';
    hexStr += String(data[i], HEX);
  }
  return hexStr;
}


// convert ASCII hex string -> bytes
void hexToBytes(const String &hex, uint8_t *out) {
  size_t outLen = hex.length() / 2;
  for (size_t i = 0; i < outLen; i++) {
    String byteStr = hex.substring(i*2, i*2 + 2);
    out[i] = (uint8_t) strtoul(byteStr.c_str(), nullptr, 16);
  }
}


// AES-ECB encrypt
void aes_encrypt(uint8_t *plainText, uint8_t *key, uint8_t *outputBuffer, size_t len = 32, size_t keyLen = 32) {

  while (len-- > 0 && plainText[len] == 0x00) {
    continue;
  }
  len = (++len + 15) & ~15;

  while (keyLen-- > 0 && key[keyLen] == 0x00) {
    continue;
  }
  keyLen = (++keyLen + 15) & ~15;

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, (const unsigned char *)key, keyLen * 8);

  // AES ECB, 16-byte blocks, pad to multiple of 16
  for (size_t i = 0; i < len; i += 16) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, (const unsigned char *)(plainText + i), (unsigned char *)(outputBuffer + i));
  }
  mbedtls_aes_free(&aes);
}


// AES-ECB decrypt
void aes_decrypt(uint8_t *chipherText, uint8_t *key, uint8_t *outputBuffer, size_t cipherLen, size_t keyLen) {

  while (cipherLen-- > 0 && chipherText[cipherLen] == 0x00) {
    continue;
  }
  cipherLen = (++cipherLen + 15) & ~15;

  while (keyLen-- > 0 && key[keyLen] == 0x00) {
    continue;
  }
  keyLen = (++keyLen + 15) & ~15;

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, (const unsigned char *)key, keyLen * 8);

  // AES ECB, 16-byte blocks, pad to multiple of 16
  for (size_t i = 0; i < cipherLen; i += 16) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char *)(chipherText + i), (unsigned char *)(outputBuffer + i));
  }
  mbedtls_aes_free(&aes);
}


// garbage hash generator
void junkHash(const uint8_t *key, uint8_t *outBuf) {
  size_t len = strlen((const char*)key);
  if (len < 16) len = 16;
  for (size_t i = 0; i < len; i++) {
    uint8_t byte = key[i];
    byte = ~byte;
    byte = ((byte & 0x0F) << 4) | ((byte & 0xF0) >> 4);
    byte ^= 0xAA;
    byte = (uint8_t)(((byte + i) * (byte + i)) % 256);
    if (byte == 0x00) {
      byte = (uint8_t)(i + 1);
    }
    outBuf[i] = byte;
  }
}


// convert ASCII base64 string -> bytes
void base64_decode(const String& b64input, uint8_t* outBuf, size_t outBufSize, size_t* outLen) {
  *outLen = outBufSize;
  mbedtls_base64_decode(outBuf, outBufSize, outLen, (const unsigned char*)b64input.c_str(), b64input.length());
}


// decrypt with private key
void rsa_decrypt(const char* keyfile, const uint8_t* encrypted, size_t olen, uint8_t* decrypted, size_t* len) {
  // initialize context
  mbedtls_pk_context pk;

  // prep operating context
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  // seed deterministic random byte generator
  int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
  if (ret != 0) {
    Serial.print("mbedtls_ctr_drbg_seed failed: -0x");
    Serial.println(-ret, HEX);
    return;
  }

  // init for decryption
  mbedtls_pk_init(&pk);

  // open private PEM file
  File privateKeyFile = LittleFS.open(keyfile, "r");
  if (!privateKeyFile) {
    Serial.print("LittleFS: cannot access '");
    Serial.print(keyfile);
    Serial.println("': No such file or directory");
    return;
  }
  size_t pem_len = privateKeyFile.size();
  unsigned char *privateKey = (unsigned char*)malloc(pem_len + 1);
  if (!privateKey) {
    privateKeyFile.close();
    Serial.println("Failed to allocate memory for private key");
    return;
  }
  privateKeyFile.read(privateKey, pem_len);
  privateKeyFile.close();
  privateKey[pem_len] = '\0';

  // read private key
  if ((ret = mbedtls_pk_parse_key(&pk, privateKey, pem_len + 1, NULL, 0, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
    Serial.print("mbedtls_pk_parse_key failed: -0x");
    Serial.println(-ret, HEX);
    free(privateKey);
    mbedtls_pk_free(&pk);
    return;
  }
  free(privateKey);

  // set OAEP + SHA1 padding before decrypt
  mbedtls_rsa_context *rsa = mbedtls_pk_rsa(pk);
  mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1);

  // decypt
  if ((ret = mbedtls_pk_decrypt(&pk, encrypted, olen, decrypted, len, olen, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
    Serial.print("Decrypting RSA failed\nmbedtls_pk_decrypt returned -0x");
    Serial.println(-ret, HEX);
  }
  mbedtls_pk_free(&pk);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
}


// generate RSA 2048-bit private.pem + public.pem key pair files
void generateKeys() {
  if (rsaKeys) return; // already checked
  bool keysValid = false;

  // check first 10 bytes match "^-----BEGIN"
  if (LittleFS.exists(privKeyFile) && LittleFS.exists(pubKeyFile)) {
    File privFile = LittleFS.open(privKeyFile, "r");
    File pubFile  = LittleFS.open(pubKeyFile, "r");
    if (privFile && pubFile) {
      char header[11] = {0};
      privFile.readBytes(header, 10);
      if (strncmp(header, "-----BEGIN", 10) == 0) {
        pubFile.readBytes(header, 10);
        if (strncmp(header, "-----BEGIN", 10) == 0) {
          keysValid = true;
        }
      }
    }
    privFile.close();
    pubFile.close();
  }

  if (!keysValid) {
    Serial.println("Generating RSA 2048-bit key pair...");

    // Initialize Mbed TLS structures
    mbedtls_pk_context pk;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    const char *pers = "rsa_gen";

    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)pers, strlen(pers)) != 0) {
      Serial.println("DRBG seed failed");
      return;
    }

    if (mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0) {
      Serial.println("PK setup failed");
      return;
    }

    // disable watchdog for this task
    esp_task_wdt_delete(NULL);

    if (mbedtls_rsa_gen_key(mbedtls_pk_rsa(pk), mbedtls_ctr_drbg_random, &ctr_drbg, 2048, 65537) != 0) {
      Serial.println("RSA key generation failed");
      esp_task_wdt_add(NULL);  // re-enable before returning
      return;
    }
    // re-enable watchdog
    esp_task_wdt_add(NULL);

    // write private key to PEM
    unsigned char privPem[1792];
    if (mbedtls_pk_write_key_pem(&pk, privPem, sizeof(privPem)) != 0) {
      Serial.println("Private key PEM export failed");
      return;
    }
    File fPriv = LittleFS.open(privKeyFile, "w");
    fPriv.write(privPem, strlen((char*)privPem));
    fPriv.close();

    // write public key to PEM
    unsigned char pubPem[512];
    if (mbedtls_pk_write_pubkey_pem(&pk, pubPem, sizeof(pubPem)) != 0) {
      Serial.println("Public key PEM export failed");
      return;
    }
    File fPub = LittleFS.open(pubKeyFile, "w");
    fPub.write(pubPem, strlen((char*)pubPem));
    fPub.close();

    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    Serial.println("RSA key pair generated.");
  }

  rsaKeys = true;
}


// send wifi credentials to HTML client
void cryptUpdateWifi() {
  uint8_t random1[32] = {0}; // encrypted challenge
  uint8_t random2[32] = {0}; // encrypted response
  uint8_t response[33] = {0};
  uint8_t pwdhash[33] = {0};

  // clear keys
  memset(enc_ssid, 0x00, sizeof(enc_ssid));
  memset(enc_pass, 0x00, sizeof(enc_pass));
  memset(fw_key, 0x00, sizeof(fw_key));
  memset(enc_seed, 0x00, sizeof(enc_seed));
  memset(pseudoKey, 0x00, sizeof(pseudoKey));
  memset(challenge, 0x00, sizeof(challenge));

  // decrypted salt
  base64_decode(salt, enc_seed, sizeof(enc_seed), &seedLen);
  rsa_decrypt(privKeyFile, enc_seed, seedLen, fw_key, &fwKeyLen);

  // decrypted challenge
  hexToBytes(seed, random1);
  aes_decrypt(random1, fw_key, challenge);
  challenge[32] = '\0';

  // encrypted response
  junkHash(fw_key, pseudoKey); // scrambled key
  junkHash(challenge, response);
  aes_encrypt(response, pseudoKey, random2);

  // converted to ASCII hex
  salt.clear();
  salt = bytesToHex(random2, sizeof(random2));

  // encrypted credentials
  aes_encrypt(ssid, fw_key, enc_ssid);
  junkHash(password, pwdhash); // scrambled password
  aes_encrypt(pwdhash, pseudoKey, enc_pass);

  // converted to ASCII hex
  hash.clear();
  seed.clear();
  hash = bytesToHex(enc_ssid, sizeof(enc_ssid));
  seed = bytesToHex(enc_pass, sizeof(enc_pass));
}


// receive wifi credentials from HTML client
bool cryptGetWifi() {
  uint8_t random1[32] = {0}; // encrypted challenge
  uint8_t random2[32] = {0}; // encrypted response
  uint8_t response[33] = {0};

  // clear keys
  memset(enc_ssid, 0x00, sizeof(enc_ssid));
  memset(enc_pass, 0x00, sizeof(enc_pass));

  // verify session
  hexToBytes(salt, random1);
  aes_encrypt(challenge, pseudoKey, random2);
  if (handshake(random1, sizeof(random1), random2, sizeof(random2))) {

    // decrypted credentials
    if (hash.length() > 31) {
      hexToBytes(hash, enc_ssid);
      memset(ssid, 0x00, sizeof(ssid));
      aes_decrypt(enc_ssid, fw_key, ssid);
      ssid[32] = '\0';
    }
    if (seed.length() > 31) {
      hexToBytes(seed, enc_pass);
      memset(password, 0x00, sizeof(password));
      aes_decrypt(enc_pass, fw_key, password);
      password[32] = '\0';
    }

    // derived key
    if (getMac(hw_key) != ESP_OK) return false;
    memset(enc_ssid, 0x00, sizeof(enc_ssid));
    memset(enc_pass, 0x00, sizeof(enc_pass));

    // encrypted credentials
    aes_encrypt(ssid, hw_key, enc_ssid);
    aes_encrypt(password, hw_key, enc_pass);

    // encrypted response
    junkHash(challenge, response);
    aes_encrypt(response, fw_key, random2);

    // converted to ASCII hex
    hash.clear();
    seed.clear();
    salt.clear();
    hash = bytesToHex(enc_ssid, sizeof(enc_ssid));
    seed = bytesToHex(enc_pass, sizeof(enc_pass));
    salt = bytesToHex(random2, sizeof(random2));

    // confirm handshake
    Serial.println("wifi credentials changed");
    return true;
  } else {
    Serial.println("Session expired");
    return false;
  }
}