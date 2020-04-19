#include <user_interface.h>
#include <espnow.h>
#include "EspNowStream.h"

bool EspNowStreamClass::begin(uint8_t channel, const uint8_t *peer_mac) {
  if (channel > 13) {
    wifi_country_t wc;

    wc.cc[0] = 'J';
    wc.cc[1] = 'P';
    wc.cc[2] = '\0';
    wc.schan = 1;
    wc.nchan = 14;
    wc.policy = WIFI_COUNTRY_POLICY_MANUAL;
    if (! wifi_set_country(&wc))
      return false;
  }
  if (wifi_get_opmode() < SOFTAP_MODE) {
    if (! wifi_set_opmode_current(wifi_get_opmode() == STATION_MODE ? STATIONAP_MODE : SOFTAP_MODE))
      return false;

    softap_config sc;

    sc.authmode = AUTH_OPEN;
    sc.ssid[0] = 0;
    sc.ssid_len = 0;
    sc.password[0] = 0;
    wifi_softap_set_config_current(&sc); // Close AP
  }
  if (channel != ESPNOW_CHANNEL_CURRENT) {
    if (! wifi_set_channel(channel))
      return false;
  }

  if (! esp_now_init()) {
    if (! esp_now_set_self_role(ESP_NOW_ROLE_COMBO)) {
      if (peer_mac)
        memcpy_P((void*)_peer_mac, peer_mac, sizeof(_peer_mac));
      else
        memset((void*)_peer_mac, 0xFF, sizeof(_peer_mac)); // Broadcast MAC
      if (! esp_now_add_peer((uint8_t*)_peer_mac, ESP_NOW_ROLE_COMBO, wifi_get_channel(), NULL, 0)) {
        if (! esp_now_register_recv_cb(esp_now_recvcb)) {
          _rx_buf_top = _rx_buf_len = 0;
          _tx_buf_len = 0;
#ifdef AUTO_FLUSH_TIME
          os_timer_setfn(&_flush_timer, flush_timercb, this);
#endif
          return true;
        }
      }
    }
    esp_now_deinit();
  }
  return false;
}

bool EspNowStreamClass::end() {
#ifdef AUTO_FLUSH_TIME
  os_timer_disarm(&_flush_timer);
#endif
  return (esp_now_deinit() == 0);
}

int EspNowStreamClass::available() {
  return _rx_buf_len;
}

int EspNowStreamClass::read() {
  if (_rx_buf_len) {
    uint8_t result = _rx_buf[_rx_buf_top];

    if (++_rx_buf_top >= RX_BUF_SIZE)
      _rx_buf_top = 0;
    --_rx_buf_len;
    return result;
  }
  return -1;
}

int EspNowStreamClass::peek() {
  if (_rx_buf_len) {
    return _rx_buf[_rx_buf_top];
  }
  return -1;
}

size_t EspNowStreamClass::readBytes(char *buffer, size_t length) {
  uint8_t result = 0;

  while (_rx_buf_len && length--) {
    buffer[result++] = _rx_buf[_rx_buf_top];
    if (++_rx_buf_top >= RX_BUF_SIZE)
      _rx_buf_top = 0;
    --_rx_buf_len;
  }
  return result;
}

size_t EspNowStreamClass::write(uint8_t data) {
  _tx_buf[_tx_buf_len++] = data;
  if (_tx_buf_len >= TX_BUF_SIZE)
    flush();
#ifdef AUTO_FLUSH_TIME
  if (_tx_buf_len) {
    os_timer_disarm(&_flush_timer);
    os_timer_arm(&_flush_timer, AUTO_FLUSH_TIME, false);
  }
#endif
  return sizeof(data);
}

size_t EspNowStreamClass::write(const uint8_t *buffer, size_t size) {
  size_t result = 0;

#ifdef AUTO_FLUSH_TIME
  os_timer_disarm(&_flush_timer);
#endif
  while (size--) {
    _tx_buf[_tx_buf_len++] = buffer[result++];
    if (_tx_buf_len >= TX_BUF_SIZE)
      flush();
  }
#ifdef AUTO_FLUSH_TIME
  if (_tx_buf_len) {
    os_timer_arm(&_flush_timer, AUTO_FLUSH_TIME, false);
  }
#endif
  return result;
}

void EspNowStreamClass::flush() {
#ifdef AUTO_FLUSH_TIME
  os_timer_disarm(&_flush_timer);
#endif
  if (_tx_buf_len) {
    esp_now_send((uint8_t*)_peer_mac, (uint8_t*)_tx_buf, _tx_buf_len);
    _tx_buf_len = 0;
  }
}

void EspNowStreamClass::esp_now_recvcb(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (_peer_mac[0] == 0xFF) { // Broadcast MAC (allow any)
    if (! esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, wifi_get_channel(), NULL, 0)) {
      memcpy((void*)_peer_mac, mac, sizeof(_peer_mac));
    }
  } else if (memcmp((void*)_peer_mac, mac, sizeof(_peer_mac))) { // Ignore unknown MAC
    return;
  }
  while (len--) {
    if (_rx_buf_len >= RX_BUF_SIZE)
      break;
    _rx_buf[(_rx_buf_top + _rx_buf_len++) % RX_BUF_SIZE] = *data++;
  }
}

#ifdef AUTO_FLUSH_TIME
void EspNowStreamClass::flush_timercb(void *self) {
  ((EspNowStreamClass*)self)->flush();
}

os_timer_t EspNowStreamClass::_flush_timer;
#endif

volatile uint8_t EspNowStreamClass::_rx_buf[EspNowStreamClass::RX_BUF_SIZE];
volatile uint8_t EspNowStreamClass::_tx_buf[EspNowStreamClass::TX_BUF_SIZE];
volatile uint8_t EspNowStreamClass::_rx_buf_top, EspNowStreamClass::_rx_buf_len;
volatile uint8_t EspNowStreamClass::_tx_buf_len;
volatile uint8_t EspNowStreamClass::_peer_mac[6];

#ifndef NO_GLOBAL_INSTANCES
EspNowStreamClass EspNowStream;
#endif
