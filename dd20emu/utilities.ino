/**
 * LED visual when Drive is enabled
 */
bool old_drv_enabled = -1; 
inline void handle_drive_enable() {
  bool isEnabled = drv_enabled;
  if (isEnabled != old_drv_enabled) {
    digitalWrite(LED_BUILTIN, isEnabled);
    old_drv_enabled = isEnabled;
    //serial_log("Enabled=%d", drv_enabled);
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
    //serial_log("Trk: %d, %s", vtech1_track_x2 / 2, wrRequest ? "Write" : "Read");
    old_wr_req = wrRequest;
  }
}

void serial_log( const char * format, ... )
{
  char buffer[256];
  va_list args;
  va_start (args, format);
  vsprintf (buffer, format, args);
  Serial.println(buffer);
  va_end (args);
}
