/**
 * LED visual when Drive is enabled
 */
bool old_drv_enabled = -1; 
inline void handle_drive_enable() {
  bool isEnabled = drv_enabled;
  if (isEnabled != old_drv_enabled) {
    digitalWrite(LED_BUILTIN, isEnabled);
    old_drv_enabled = isEnabled;
    //serial_log(PSTR("Enabled=%d"), drv_enabled);
  }
}

/**
 * Serial log on R/W request
 */
bool old_wr_req = -1;
void handle_wr_request() {
  if (!drv_enabled)
    return;

  bool wrRequest = write_request;
  if (wrRequest != old_wr_req) {
    //serial_log(PSTR("Trk: %d, %s"), vtech1_track_x2 / 2, wrRequest ? "Write" : "Read");
    old_wr_req = wrRequest;
  }
}

void serial_log( const char * format, ... )
{
  char buffer[256];
  va_list args;
  va_start (args, format);
#ifdef __AVR__
    vsnprintf_P(buffer, sizeof(buffer), (const char *)format, args);     // progmem for AVR
#else
    vsnprintf(buffer, sizeof(buffer), (const char *)format, args);     // for the rest of the world
#endif  
//  vsprintf (buffer, format, args);
  Serial.print(buffer);
  va_end (args);
}
