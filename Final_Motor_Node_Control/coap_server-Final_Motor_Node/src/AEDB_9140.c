#include "AEDB_9140.h"
#include "zephyr/kernel.h"
#include <math.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
//#include <zephyr/logging/log.h>

//LOG_MODULE_REGISTER(encoder, CONFIG_ENCODER_LOG_LEVEL);
static const struct gpio_dt_spec ChannelA_Encoder =
    GPIO_DT_SPEC_GET(DT_NODELABEL(encodercha), gpios);
static const struct gpio_dt_spec ChannelB_Encoder =
    GPIO_DT_SPEC_GET(DT_NODELABEL(encoderchb), gpios);
static const struct gpio_dt_spec ChannelI_Encoder =
    GPIO_DT_SPEC_GET(DT_NODELABEL(encoderindex), gpios);

static struct gpio_callback encoderA_callback;
static struct gpio_callback encoderB_callback;
static struct gpio_callback encoderI_callback;

static const int QEM[4][4] = {{0, -1, 1, 2},
                              {1, 0, 2, -1},
                              {-1, 2, 0, 1},
                              {2, 1, -1, 0}}; // Quadrature Encoder Matrix
int state = 0;
int oldState = 0;
int32_t position = 0;
int32_t position_prevfullrev = 0;
int32_t FullResolutionPositionCount = 0;
int32_t intVel = 0;
float floatVel = 0.0f;
int32_t intAcc = 0;
float floatAcc = 0.0f;

int aState = 0;
int bState = 0;

int32_t getIntVel(void) { return intVel; }
float getFloatVel(void) { return floatVel; }
int32_t getIntAcc(void) { return intAcc; }
float getFloatAcc(void) { return floatAcc; }

void setPosition(struct k_work *work) {
  static int oldState = 0;
  state = (bState << 1) + aState;
  position += QEM[state][oldState];
  oldState = state;
}
K_WORK_DEFINE(setPosition_work, setPosition);
void aChange() {
  aState = gpio_pin_get_dt(&ChannelA_Encoder);
  k_work_submit(&setPosition_work);
}
void bChange() {
  bState = gpio_pin_get_dt(&ChannelB_Encoder);
  k_work_submit(&setPosition_work);
}

void FullRotationCounter() {
  FullResolutionPositionCount = position - position_prevfullrev;
  position_prevfullrev = position;
}

void measure(struct k_work *work) {
  static uint8_t count = 0;
  setVelocity();
  if (count % 2 == 0) {
    setAcceleration();
  }
}
K_WORK_DEFINE(measure_work, measure);

void measureInterrupt(struct k_timer *timer_id) {
  k_work_submit(&measure_work);
}
K_TIMER_DEFINE(measureTime, measureInterrupt, NULL);

void Setup_interrupt(void) {
  gpio_pin_interrupt_configure_dt(&ChannelA_Encoder, GPIO_INT_EDGE_BOTH);
  gpio_pin_interrupt_configure_dt(&ChannelB_Encoder, GPIO_INT_EDGE_BOTH);
  gpio_init_callback(&encoderA_callback, aChange, BIT(ChannelA_Encoder.pin));
  gpio_add_callback(ChannelA_Encoder.port, &encoderA_callback);
  gpio_init_callback(&encoderB_callback, bChange, BIT(ChannelB_Encoder.pin));
  gpio_add_callback(ChannelB_Encoder.port, &encoderB_callback);
  k_timer_start(&measureTime, K_SECONDS(0), K_MSEC(ENCODER_SAMPLE_PERIOD_MS));
}

void resetPosition(int32_t in) { position = in; return; }

int32_t getPosition(void) { return position; }

// Should be called every period.
void setVelocity(void) {
  static int32_t previousPosition = 0;
  intVel = position - previousPosition;
  floatVel =
      (float)intVel * (40.85E-6) / (float)ENCODER_SAMPLE_PERIOD_MS * 1000.0f;
  previousPosition = position;
}
// Should be called every 3rd period.
void setAcceleration(void) {
  static int32_t previousVelocity = 0;
  static float previousFloatVel = 0;
  intAcc = intVel - previousVelocity;
  floatAcc = (floatVel - previousFloatVel) / (float)ENCODER_SAMPLE_PERIOD_MS /
             3.0f * 1000.0f;
  previousVelocity = intVel;
  previousFloatVel = floatVel;
}

void encoderTestLoop(void) {
  for (int i = 1;; i++) {
    k_msleep((int32_t)(ENCODER_SAMPLE_PERIOD_MS * 1000.0f));
    LOG_DBG("Pos: %d, Vel: %.3f, Acc: %3f\n", getPosition(), getFloatVel(),
            getFloatAcc());
    k_msleep((int32_t)(ENCODER_SAMPLE_PERIOD_MS));
    setVelocity();
    if (i % 3 == 0) {
      setAcceleration();
      i = 0;
    }
  }
}
