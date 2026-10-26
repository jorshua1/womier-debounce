#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#define DEV_IF0 "/dev/input/by-id/usb-SEMICO_Gaming_Keyboard-event-kbd"
#define DEBOUNCE_MS 35

static volatile int running = 1;

void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

static uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static inline int is_modifier(uint16_t code) {
    return (code == KEY_LEFTALT || code == KEY_RIGHTALT ||
            code == KEY_LEFTCTRL || code == KEY_RIGHTCTRL ||
            code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT ||
            code == KEY_LEFTMETA || code == KEY_RIGHTMETA ||
            code == KEY_TAB || code == KEY_CAPSLOCK);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("[Womier-Debounce] Esperando 500ms para que se libere la tecla Enter del comando sudo...\n");
    // CRÍTICO: Esperar a que el usuario levante el dedo del Enter usado para ejecutar sudo.
    // De lo contrario, EVIOCGRAB secuestra el teclado mientras Enter está abajo,
    // y Wayland/KWin nunca recibe la liberación (KeyUp), dejando el Enter atascado.
    usleep(500000);

    int fd0 = open(DEV_IF0, O_RDONLY | O_NONBLOCK);
    if (fd0 < 0) {
        fprintf(stderr, "Error abriendo %s: %s (¿Tienes permisos sudo?)\n", DEV_IF0, strerror(errno));
        return 1;
    }

    struct libevdev *dev0 = NULL;
    if (libevdev_new_from_fd(fd0, &dev0) < 0) {
        fprintf(stderr, "Error inicializando libevdev desde %s\n", DEV_IF0);
        close(fd0);
        return 1;
    }

    // Vaciar cualquier evento residual (como el KeyUp del Enter de sudo) antes de secuestrar
    struct input_event drain_ev;
    while (read(fd0, &drain_ev, sizeof(drain_ev)) > 0);

    // Ahora sí, secuestro exclusivo del teclado físico
    if (ioctl(fd0, EVIOCGRAB, 1) < 0) {
        fprintf(stderr, "Error haciendo EVIOCGRAB: %s\n", strerror(errno));
        libevdev_free(dev0);
        close(fd0);
        return 1;
    }
    printf("[Womier-Debounce] Teclado físico secuestrado en estado limpio.\n");

    struct libevdev_uinput *uidev = NULL;
    if (libevdev_uinput_create_from_device(dev0, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev) < 0) {
        fprintf(stderr, "Error creando clon virtual uinput: %s\n", strerror(errno));
        ioctl(fd0, EVIOCGRAB, 0);
        libevdev_free(dev0);
        close(fd0);
        return 1;
    }
    printf("[Womier-Debounce] Clon uinput activo: %s (Device: %s)\n",
           libevdev_get_name(dev0), libevdev_uinput_get_devnode(uidev));

    // Por seguridad, emitir un KeyUp explícito para Enter y modificadores
    // para asegurar que Wayland empiece con un estado 100% libre
    libevdev_uinput_write_event(uidev, EV_KEY, KEY_ENTER, 0);
    libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);

    // Estado lógico y tiempos por tecla
    int logical_down[KEY_CNT] = {0};
    uint64_t last_down_time[KEY_CNT] = {0};
    uint64_t last_up_time[KEY_CNT] = {0};

    printf("[Womier-Debounce] Filtro simétrico activo (35 ms). Presiona Ctrl+C para salir.\n");

    struct input_event ev;
    while (running) {
        int rc = libevdev_next_event(dev0, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            while (rc == LIBEVDEV_READ_STATUS_SYNC) {
                rc = libevdev_next_event(dev0, LIBEVDEV_READ_FLAG_SYNC, &ev);
            }
            continue;
        } else if (rc != LIBEVDEV_READ_STATUS_SUCCESS) {
            if (errno == EINTR) break;
            usleep(1000);
            continue;
        }

        uint64_t now = get_time_ms();

        if (ev.type == EV_KEY) {
            uint16_t code = ev.code;
            if (code >= KEY_CNT) continue;

            if (ev.value == 1) { // KeyDown
                if (logical_down[code]) {
                    // Ya está abajo lógicamente, ignorar vibración interna
                    continue;
                }
                if (!is_modifier(code) && last_up_time[code] > 0 && (now - last_up_time[code] < DEBOUNCE_MS)) {
                    // Rebote posterior a liberación descartado (solo para teclas normales de escritura)
                    continue;
                }

                // Pulsación limpia
                logical_down[code] = 1;
                last_down_time[code] = now;

                libevdev_uinput_write_event(uidev, EV_KEY, code, 1);
                libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);

                if (code == KEY_ENTER) {
                    printf("-> [OK] Enter KeyDown enviado al sistema\n");
                    fflush(stdout);
                }

            } else if (ev.value == 0) { // KeyUp
                if (!logical_down[code]) {
                    // Ya estaba arriba lógicamente, ignorar
                    continue;
                }

                // Liberación limpia: NUNCA se descarta un KeyUp
                // Descartar un KeyUp deja la tecla (o modificadores como Shift) pegada en el SO.
                logical_down[code] = 0;
                last_up_time[code] = now;

                libevdev_uinput_write_event(uidev, EV_KEY, code, 0);
                libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);

                if (code == KEY_ENTER) {
                    printf("-> [OK] Enter KeyUp enviado al sistema (duración=%lums)\n", now - last_down_time[code]);
                    fflush(stdout);
                }
            } else if (ev.value == 2) { // Repeat
                if (logical_down[code]) {
                    libevdev_uinput_write_event(uidev, EV_KEY, code, 2);
                    libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
                }
            }
        } else if (ev.type != EV_SYN) {
            // Reenviar MSC_SCAN u otros eventos
            libevdev_uinput_write_event(uidev, ev.type, ev.code, ev.value);
            libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
        }
    }

    printf("\n[Womier-Debounce] Restaurando teclado físico y saliendo...\n");
    ioctl(fd0, EVIOCGRAB, 0);
    libevdev_uinput_destroy(uidev);
    libevdev_free(dev0);
    close(fd0);
    return 0;
}
