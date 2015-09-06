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

    long int recv_ctrl;
    double recv_battery, recv_phev;

    startServers();

    for(i = 0; i < 19; ++i) {
        sendOMcontrol(control);

        ++control;

        sendOM(values[i][0], "energy");
        sendOM(values[i][1], "production");
        sendOM(values[i][2], "consumption");
        sendOM(values[i][3], "battery");

        fprintf(stderr, "Sent 4 values.\n");

        recv_ctrl = getOMcontrol();

        fprintf(stderr, "Received control %ld.\n", recv_ctrl);

        recv_battery = getOM("battery"); 
        recv_phev = getOM("phev");

        fprintf(stderr, "Received battery & phev: %f %f.\n", recv_battery, recv_phev);
    }
}
