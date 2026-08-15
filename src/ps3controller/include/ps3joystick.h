#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_
#include "rtwtypes.h"

// Button state
typedef struct {
    int buttons[17];
    short axis[6];
} joystick_state;

int joystickSetup();
void readJoystick(int fd, joystick_state *j_state);
int joystickTerminate(int fd);

#endif //_JOYSTICK_H_