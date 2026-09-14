/*                                                                          *
 * Mbed TLS Knowledge Base                                                  *
 *                                                                          *
 * RSA 2048-bit decryption in C with Mbed TLS                               *
 *                                                                          *
 * mbed-tls.readthedocs.io/en/latest/kb/how-to/encrypt-and-decrypt-with-rsa *
 *                                                                          *
 *                                                                          */
#ifndef CRYPTO_H
#define CRYPTO_H


#include <Arduino.h>
#include <LittleFS.h>
#include <esp_mac.h>
#include <esp_task_wdt.h>
#include <mbedtls/base64.h>
#include <mbedtls/aes.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pem.h>

extern uint8_t ssid[33];
extern uint8_t password[33];
extern uint8_t enc_ssid[32];
extern uint8_t enc_pass[32];
extern uint8_t hw_key[16];
extern String hash;
extern String seed;
extern String salt;

inline constexpr const char* pubKeyFile = "/public.pem";
inline constexpr const char* privKeyFile = "/private.pem";


// read MAC from eFuse
esp_err_t getMac(uint8_t* key);

// convert ASCII hex string -> bytes
void hexToBytes(const String &hex, uint8_t *out);

// AES-ECB decrypt
void aes_decrypt(uint8_t *chipherText, uint8_t *key, uint8_t *outputBuffer, size_t cipherLen = 32, size_t keyLen = 32);

// generate RSA 2048-bit private.pem + public.pem key pair files
void generateKeys();

// send wifi credentials to HTML client
void cryptUpdateWifi();

// receive wifi credentials from HTML client
bool cryptGetWifi();


// internal functions
/*
// convert bytes -> ASCII hex string
String bytesToHex(const uint8_t *data, size_t len);

// convert ASCII base64 string -> bytes
void base64_decode(const String& b64input, uint8_t* outBuf, size_t outBufSize, size_t* outLen);

// decrypt with private key
void rsa_decrypt(const char* keyfile, const uint8_t* encrypted, size_t olen, uint8_t* decrypted, size_t* len);

// AES-ECB encrypt
void aes_encrypt(uint8_t *plainText, uint8_t *key, uint8_t *outputBuffer, size_t len = 32, size_t keyLen = 32);

// garbage hash generator
void junkHash(const uint8_t *key, uint8_t *outBuf);

// compare two arrays
bool handshake(const uint8_t *a, size_t size_a, const uint8_t *b, size_t size_b);
*/


#endif /* CRYPTO_H */