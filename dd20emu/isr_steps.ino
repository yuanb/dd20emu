extern byte current_track;
extern uint8_t vtech1_fdc_latch;
extern uint8_t vtech1_track_x2;

void handle_step0() {
  //if (!drv_enabled)
  //  return;
 
  uint8_t data = PIN_STEP_REG & 0x0F;

  //Track--, if current bit is shifted to right by 1, wrapped by 4 bits
  if (data == 0x08 && PHI0(vtech1_fdc_latch) && vtech1_track_x2 >0)  {
    vtech1_track_x2--;
  }
  //Track++, if current bit is shifted to left by 1, wrapped by 4 bits
  else if (data == 0x08 && PHI2(vtech1_fdc_latch) && vtech1_track_x2 < 80) {
    vtech1_track_x2++;  
  }
  Serial.println(vtech1_track_x2); 
  vtech1_fdc_latch = data;
}

void handle_step1() {
  //if (!drv_enabled)
  //  return;

  uint8_t data = PIN_STEP_REG & 0x0F;
  if (data == 0x04 && PHI3(vtech1_fdc_latch))  {
    if (vtech1_track_x2 > 0)
      vtech1_track_x2--;
  }
  else if (data == 0x04 && PHI1(vtech1_fdc_latch)) {
    if (vtech1_track_x2 < 80)
      vtech1_track_x2++;      
  }
  Serial.println(vtech1_track_x2);   
  vtech1_fdc_latch = data;
}

void handle_step2() {
  //if (!drv_enabled)
  //  return;

  uint8_t data = PIN_STEP_REG & 0x0F;
  if (data == 0x02 && PHI2(vtech1_fdc_latch))  {
    if (vtech1_track_x2 > 0)
      vtech1_track_x2--;
  }
  else if (data == 0x02 && PHI0(vtech1_fdc_latch)) {
    if (vtech1_track_x2 < 80)
      vtech1_track_x2++;      
  }
  Serial.println(vtech1_track_x2);   
  vtech1_fdc_latch = data;
}

void handle_step3() {
  //if (!drv_enabled)
  //  return;

  uint8_t data = PIN_STEP_REG & 0x0F;
  if (data == 0x01 && PHI1(vtech1_fdc_latch))  {
    if (vtech1_track_x2 > 0)
      vtech1_track_x2--;  
  }
  else if (data == 0x01 && PHI3(vtech1_fdc_latch)) {
    if (vtech1_track_x2 < 80)
      vtech1_track_x2++;       
  }
  Serial.println(vtech1_track_x2);   
  vtech1_fdc_latch = data;
}
