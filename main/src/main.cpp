#include "utils.h"
#include "pid.h"
#include "internet.h"

void setup()
{
    setupSystem();
}

void loop()
{
    controlLoop();
}
