#include "harmonizer_app.h"

int main(int argc, char **argv) {
    init_harmonizer_app(argc, argv);
    run_harmonizer_app();
    destroy_harmonizer_app();
}
