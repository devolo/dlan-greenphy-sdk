typedef struct {
    uint8_t  safety_flag;       // 0=OFF, 1=ON
    uint8_t  pwm_state;         // 0=No current, 1=Charging
    uint8_t  battery_percent;   // 0~100%
} VehicleStatus_t;
