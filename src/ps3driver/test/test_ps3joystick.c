/*
 * Unit tests for ps3joystick.c
 *
 * Uses a non-blocking pipe to emulate /dev/input/jsX: struct js_event
 * records are written to the pipe's write end and read back through the
 * driver exactly as it reads the real device (O_NONBLOCK; read() returns
 * -1/EAGAIN once drained, matching a real joystick fd with nothing pending).
 *
 * Build & run on the target (needs the real linux/joystick.h):
 *   gcc -Wall -I../include -o test_ps3joystick test_ps3joystick.c ../src/ps3joystick.c
 *   ./test_ps3joystick
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <ps3joystick.h>

extern int read_event(int fd, struct js_event *event);
extern void readJoystick(int fd, joystick_state *j_state);

static int open_test_pipe(int *read_fd)
{
    int fds[2];
    if (pipe(fds) == -1) {
        perror("pipe");
        return -1;
    }
    if (fcntl(fds[0], F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    *read_fd = fds[0];
    return fds[1];
}

static void push_event(int write_fd, unsigned int time, __s16 value, __u8 type, __u8 number)
{
    struct js_event e;
    e.time = time;
    e.value = value;
    e.type = type;
    e.number = number;
    if (write(write_fd, &e, sizeof(e)) != (ssize_t)sizeof(e)) {
        perror("write");
    }
}

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { \
        tests_failed++; \
        printf("FAIL: %s (%s:%d) - %s\n", __func__, __FILE__, __LINE__, msg); \
    } \
} while (0)

static void test_even_axis_is_stored_as_is(void)
{
    int rfd, wfd;
    joystick_state st;
    memset(&st, 0, sizeof(st));
    wfd = open_test_pipe(&rfd);

    push_event(wfd, 0, 12345, JS_EVENT_AXIS, 0);
    readJoystick(rfd, &st);

    CHECK(st.axis[0] == 12345, "even-numbered axis should be stored unmodified");

    close(rfd);
    close(wfd);
}

static void test_odd_axis_is_negated(void)
{
    int rfd, wfd;
    joystick_state st;
    memset(&st, 0, sizeof(st));
    wfd = open_test_pipe(&rfd);

    push_event(wfd, 0, 12345, JS_EVENT_AXIS, 1);
    readJoystick(rfd, &st);

    CHECK(st.axis[1] == -12345, "odd-numbered axis should be negated");

    close(rfd);
    close(wfd);
}

static void test_button_event_is_stored(void)
{
    int rfd, wfd;
    joystick_state st;
    memset(&st, 0, sizeof(st));
    wfd = open_test_pipe(&rfd);

    push_event(wfd, 0, 1, JS_EVENT_BUTTON, 5);
    readJoystick(rfd, &st);

    CHECK(st.buttons[5] == 1, "button state should be stored at its index");

    close(rfd);
    close(wfd);
}

static void test_last_event_wins_per_index(void)
{
    int rfd, wfd;
    joystick_state st;
    memset(&st, 0, sizeof(st));
    wfd = open_test_pipe(&rfd);

    push_event(wfd, 0, 100, JS_EVENT_AXIS, 0);
    push_event(wfd, 1, 200, JS_EVENT_AXIS, 0);
    readJoystick(rfd, &st);

    CHECK(st.axis[0] == 200, "later event for the same axis should overwrite the earlier one");

    close(rfd);
    close(wfd);
}

static void test_drains_all_pending_events_in_one_call(void)
{
    int rfd, wfd;
    joystick_state st;
    memset(&st, 0, sizeof(st));
    wfd = open_test_pipe(&rfd);

    push_event(wfd, 0, 1, JS_EVENT_BUTTON, 0);
    push_event(wfd, 1, 1, JS_EVENT_BUTTON, 16);
    push_event(wfd, 2, -32767, JS_EVENT_AXIS, 5);
    readJoystick(rfd, &st);

    CHECK(st.buttons[0] == 1, "first button in the batch should be stored");
    CHECK(st.buttons[16] == 1, "last valid button index (16) should be stored");
    CHECK(st.axis[5] == 32767, "last valid axis index (5) should be stored negated, since it is odd (-(-32767) == 32767)");

    close(rfd);
    close(wfd);
}

static void test_out_of_range_indices_are_ignored(void)
{
    /*
     * event.number comes straight from the kernel as an unsigned byte
     * (0-255). buttons[] has 17 slots and axis[] has 6; a controller
     * reporting more (or a corrupt event) must not write past those
     * arrays or corrupt neighboring fields in joystick_state.
     */
    int rfd, wfd;
    joystick_state st;
    int i;
    int all_zero = 1;
    memset(&st, 0, sizeof(st));
    wfd = open_test_pipe(&rfd);

    push_event(wfd, 0, 1, JS_EVENT_BUTTON, 17);   /* one past the last valid button index (16) */
    push_event(wfd, 1, 12345, JS_EVENT_AXIS, 6);  /* one past the last valid axis index (5) */
    push_event(wfd, 2, 1, JS_EVENT_BUTTON, 255);  /* max value of the underlying u8 field */
    readJoystick(rfd, &st);

    for (i = 0; i < 17; i++) {
        if (st.buttons[i] != 0) all_zero = 0;
    }
    for (i = 0; i < 6; i++) {
        if (st.axis[i] != 0) all_zero = 0;
    }
    CHECK(all_zero, "out-of-range event.number must not write into buttons[]/axis[] or corrupt neighboring fields");

    close(rfd);
    close(wfd);
}

static void test_no_pending_data_returns_without_blocking(void)
{
    int rfd, wfd;
    joystick_state st;
    memset(&st, 0, sizeof(st));
    wfd = open_test_pipe(&rfd);

    /* No events written: on a real device this is the normal steady state
     * (O_NONBLOCK read() -> -1/EAGAIN). readJoystick must return promptly
     * rather than blocking or spinning. */
    readJoystick(rfd, &st);

    CHECK(st.axis[0] == 0 && st.buttons[0] == 0, "state should remain untouched when nothing was pending");

    close(rfd);
    close(wfd);
}

static void test_init_flagged_events_are_processed(void)
{
    /*
     * Per the kernel joystick API, on open the driver receives one
     * synthetic event per axis/button, with JS_EVENT_INIT (0x80) OR'd into
     * the type field, reporting the controller's current resting state.
     * readJoystick() must mask off JS_EVENT_INIT before dispatching, so
     * these startup events are handled the same as ordinary ones instead of
     * being silently dropped.
     */
    int rfd, wfd;
    joystick_state st;
    memset(&st, 0, sizeof(st));
    wfd = open_test_pipe(&rfd);

    push_event(wfd, 0, 500, JS_EVENT_AXIS | JS_EVENT_INIT, 2);
    push_event(wfd, 0, 1, JS_EVENT_BUTTON | JS_EVENT_INIT, 3);
    readJoystick(rfd, &st);

    CHECK(st.axis[2] == 500, "JS_EVENT_INIT-flagged axis events should be processed like ordinary axis events");
    CHECK(st.buttons[3] == 1, "JS_EVENT_INIT-flagged button events should be processed like ordinary button events");

    close(rfd);
    close(wfd);
}

static void test_joystickSetup_and_terminate_with_real_device(void)
{
    /*
     * Exercises joystickSetup()/joystickTerminate() against whatever is at
     * /dev/input/js0. Skips (not a failure) when no joystick is attached,
     * since this depends on real hardware rather than the pipe fixture used
     * by the other tests.
     */
    int fd;

    if (access("/dev/input/js0", F_OK) != 0) {
        printf("SKIP: test_joystickSetup_and_terminate_with_real_device (no /dev/input/js0 present)\n");
        return;
    }

    fd = joystickSetup();
    CHECK(fd >= 0, "joystickSetup() should return a valid (non-negative) file descriptor when /dev/input/js0 exists");

    if (fd >= 0) {
        CHECK(joystickTerminate(fd) == 0, "joystickTerminate() should return 0");
    }
}

int main(void)
{
    test_even_axis_is_stored_as_is();
    test_odd_axis_is_negated();
    test_button_event_is_stored();
    test_last_event_wins_per_index();
    test_drains_all_pending_events_in_one_call();
    test_out_of_range_indices_are_ignored();
    test_no_pending_data_returns_without_blocking();
    test_init_flagged_events_are_processed();
    test_joystickSetup_and_terminate_with_real_device();

    printf("\n%d/%d checks passed\n", tests_run - tests_failed, tests_run);
    return tests_failed == 0 ? 0 : 1;
}
