void canEvent0(void *pvParameters) {
  while (true) {
    if (canInterruptFlag0) {
      canInterruptFlag0 = false;
      uint8_t irq = Can0.getInterrupts();
      if (irq & MCP2515::CANINTF_RX0IF) {
        if (Can0.readMessage(MCP2515::RXB0, &canMsg0) == MCP2515::ERROR_OK) {
          // export CAN_message into bit field decoder
          exportMsg(canMsg0.can_id, canMsg0.data, canMsg0.can_dlc);
        }
      }
      if (irq & MCP2515::CANINTF_RX1IF) {
        if (Can0.readMessage(MCP2515::RXB1, &canMsg0) == MCP2515::ERROR_OK) {
          // export CAN_message into bit field decoder
          exportMsg(canMsg0.can_id, canMsg0.data, canMsg0.can_dlc);
        }
      }
      Can0.clearInterrupts();
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void canEvent1(void *pvParameters) {
  while (true) {
    if (canInterruptFlag1) {
      canInterruptFlag1 = false;
      uint8_t irq = Can1.getInterrupts();
      if (irq & MCP2515::CANINTF_RX0IF) {
        if (Can1.readMessage(MCP2515::RXB0, &canMsg1) == MCP2515::ERROR_OK) {
          // export CAN_message into bit field decoder
          exportMsg(canMsg1.can_id, canMsg1.data, canMsg1.can_dlc);
        }
      }
      if (irq & MCP2515::CANINTF_RX1IF) {
        if (Can1.readMessage(MCP2515::RXB1, &canMsg1) == MCP2515::ERROR_OK) {
          // export CAN_message into bit field decoder
          exportMsg(canMsg1.can_id, canMsg1.data, canMsg1.can_dlc);
        }
      }
      Can1.clearInterrupts();
      awake(500); // prevent idle timeout
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}