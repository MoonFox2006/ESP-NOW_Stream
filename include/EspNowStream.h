#ifndef __ESPNOWSTREAM_H
#define __ESPNOWSTREAM_H

#define AUTO_FLUSH_TIME 50 // Comment this define for manual flush

#ifdef AUTO_FLUSH_TIME
#include <os_type.h>
#endif
#include <Stream.h>

#define ESPNOW_CHANNEL_CURRENT  0

class EspNowStreamClass : public Stream {
public:
  EspNowStreamClass() {}
  virtual ~EspNowStreamClass() {
    end();
  }

  bool begin(uint8_t channel = ESPNOW_CHANNEL_CURRENT, const uint8_t *peer_mac = NULL);
  bool end();

  int available();
  int read();
  int peek();
  size_t readBytes(char *buffer, size_t length);

  using Print::write;
  size_t write(uint8_t data);
  size_t write(const uint8_t *buffer, size_t size);
  void flush();

protected:
  static const uint8_t RX_BUF_SIZE = 64;
  static const uint8_t TX_BUF_SIZE = 16;

  static void esp_now_recvcb(uint8_t *mac, uint8_t *data, uint8_t len);
#ifdef AUTO_FLUSH_TIME
  static void flush_timercb(void *self);

  static os_timer_t _flush_timer;
#endif

  static volatile uint8_t _rx_buf[RX_BUF_SIZE];
  static volatile uint8_t _tx_buf[TX_BUF_SIZE];
  static volatile uint8_t _rx_buf_top, _rx_buf_len;
  static volatile uint8_t _tx_buf_len;
  static volatile uint8_t _peer_mac[6];
};

#ifndef NO_GLOBAL_INSTANCES
extern EspNowStreamClass EspNowStream;
#endif

#endif
