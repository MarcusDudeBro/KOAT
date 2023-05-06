#define G 9.80665
#define CAL_T 5000
#define PRE_CAL_T 3
#define DRIFT_THRESH 0.02
#define MAX_DRIFT_COUNT 9

struct IMU_Vals {
  float x;
  float y;
  float vy;
  float vx;
  float biasX;
  float biasY;
  float degX;
  float degY;
  float biasDegX;
  float biasDegY;
  float driftVY;
  float driftVX;
};
