#define PIN_EN_REG      PINE
#define PIN_EN_BIT      5
#define PIN_EN_MASK     0x20      //1 << PIN_EN_MASK
#define PIN_WR_REG      PINE
#define PIN_WRREQ_BIT   4
#define PIN_WRREQ_MASK  0x10      //1 << PIN_WRREQ_BIT

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
