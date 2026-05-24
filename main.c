#include <stdio.h>
#include "tarsau.h"

int main(int argc, char *argv[]) {
    
    TarsauApp *app = TarsauApp_create();
    
    app->run(app, argc, argv);
    
    TarsauApp_destroy(app);
    
    return 0;
}
