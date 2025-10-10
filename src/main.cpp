#include <iostream>
#include <cstdint>
/*
https://k3xec.com/packrat-processing-iq/
https://htorp.folk.ntnu.no/Undervisning/TTK10/IQdemodulation.pdf
https://en.wikipedia.org/wiki/In-phase_and_quadrature_components
*/
int main()
{
    uint8_t iq[2];
    while(std::cin.read(reinterpret_cast<char*>(iq),2))
    {
        float i = ((float)iq[0] - 127.5f) / 127.5f;
        float q = ((float)iq[1] - 127.5f) / 127.5f;
    }
    return 0;
}