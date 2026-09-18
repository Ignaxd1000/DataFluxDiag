#include "cli.h"

/* App main entry used by the real executable and tests if needed. */
int sd_main(int argc, char **argv) {
    return sd_cli_run(argc, argv);
}
