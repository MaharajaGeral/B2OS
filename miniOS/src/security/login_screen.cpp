#include "security/login_screen.h"
#include "security/access.h"
#include "shell/shell_io.h"
#include "kernel/console.h"

namespace login_screen {

void show() {
    char username[32];
    char password[32];

    for (;;) {
        kernel_print("\n");
        kernel_print("================================\n");
        kernel_print("           B2OS LOGIN\n");
        kernel_print("================================\n");

        kernel_print("Username: ");
        shell_io::read_line(username, sizeof(username));

        kernel_print("Password: ");
        shell_io::read_line(password, sizeof(password));

        if (access::login(username, password)) {
            kernel_print("\nLogin berhasil. Selamat datang, ");
            kernel_print(access::current_name());
            kernel_print("!\n");
            return;
        }

        kernel_print("\nLogin gagal. Silakan coba lagi.\n");
    }
}

}