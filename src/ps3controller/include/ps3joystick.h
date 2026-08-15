#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_
#include <stdint.h>

/* Button and axis state.
 *
 * The field types must match the struct declared in PS3.m, which passes this
 * type to the generated code as 'extern'. Coder does not check the header, so
 * a mismatch here is silent.
 */
typedef struct {
    uint8_t buttons[17];
    int16_t axis[6];
} joystick_state;

int joystickSetup();
void readJoystick(int fd, joystick_state *j_state);
int joystickTerminate(int fd);

#endif //_JOYSTICK_H_