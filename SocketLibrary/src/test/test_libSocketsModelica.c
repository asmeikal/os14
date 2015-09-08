#include <libSocketsModelica.h>

#include <stdlib.h>
#include <stdio.h>

double values[19][4] = {
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0},
    {4.0, 1.0, 0.5, 1.0}
};

int main(void)
{
    long int control = 1;
    int i;
    double t = 0.0;

    long int recv_ctrl;
    double recv_battery;

    startServers(t);

    for(i = 0; i < 19; ++i) {
        sendOMcontrol(control, t);

        ++control;

        sendOM(values[i][0], "energy", t);
        sendOM(values[i][1], "production", t);
        sendOM(values[i][2], "consumption", t);
        sendOM(values[i][3], "battery", t);

        fprintf(stderr, "Sent 4 values.\n");

        recv_ctrl = getOMcontrol(t);

        fprintf(stderr, "Received control %ld.\n", recv_ctrl);

        recv_battery = getOM("battery", t); 

        fprintf(stderr, "Received battery: %f.\n", recv_battery);
    }
}
