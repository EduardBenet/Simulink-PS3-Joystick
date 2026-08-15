/**
* https://www.kernel.org/doc/Documentation/input/joystick-api.txt
*/
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <ps3joystick.h>

/**
* Reads a joystick event from the joystick device.
*
* Returns 0 on success. Otherwise -1 is returned.
*/
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == -1)
        return 0;

    if (bytes == sizeof(*event))
        return 1;

    printf("Unexpected bytes from joystick:%zd\n", bytes);

    /* Error, could not read full event. */
    return -1;
}

/*
Setup joystick
*/
int joystickSetup()
{
    int js;
    const char *device;

    device = "/dev/input/js0";
    js = open(device, O_RDONLY | O_NONBLOCK);

    if (js == -1)
        perror("Could not open joystick");

    return js;
}

void readJoystick(int fd, joystick_state *j_state ) {

    struct js_event event;

    while (read_event(fd, &event) > 0)
    {
        switch (event.type & ~JS_EVENT_INIT)
        {
            case JS_EVENT_BUTTON:
                if (event.number < sizeof(j_state->buttons) / sizeof(j_state->buttons[0])) {
                    j_state->buttons[event.number] = event.value;
                }
                break;
            case JS_EVENT_AXIS:
                if (event.number < sizeof(j_state->axis) / sizeof(j_state->axis[0])) {
                    if (event.number % 2 == 0) {
                        j_state->axis[event.number] = event.value;
                    } else {
                        j_state->axis[event.number] = -event.value;
                    }
                }
                break;
            default:
                break;
        }

    }

    return;
}

int joystickTerminate(int fd)
{
    close(fd);
    return(0);
}