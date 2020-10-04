
bool drv_enabled = false;
bool write_request = false;

//Arduino interruption on pin change, nice Arduino interrupt tutorial
//https://arduino.stackexchange.com/questions/8758/arduino-interruption-on-pin-change
void driveEnabled() {
  drv_enabled = !(PIN_EN_REG & PIN_EN_MASK);
}

void writeRequest() {
  write_request = !(PIN_WR_REG & PIN_WRREQ_MASK);
}
