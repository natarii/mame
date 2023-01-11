// license:CC0-1.0
// copyright-holders:hap
// thanks-to:=CO=Windler
/*

Electroplay Sound FX Phasor (hh_pic16.cpp)
3-bit sound with volume envelope

TODO:
- transistors should be BC183

*/

#include "netlist/devices/net_lib.h"


NETLIST_START(sfxphasor)
{
	SOLVER(Solver, 48000)
	PARAM(Solver.ACCURACY, 1e-7)
	ANALOG_INPUT(VBATT, 5)

	NET_MODEL("OPENDRAIN FAMILY(TYPE=MOS OVL=0.4 OVH=2.4 ORL=1.0 ORH=1e12)")

	LOGIC_INPUT(P10, 0, "OPENDRAIN")
	LOGIC_INPUT(P13, 0, "OPENDRAIN")
	LOGIC_INPUT(P15, 0, "OPENDRAIN")
	LOGIC_INPUT(P16, 0, "OPENDRAIN")
	LOGIC_INPUT(P17, 0, "OPENDRAIN")
	NET_C(VBATT, P10.VDD, P13.VDD, P15.VDD, P16.VDD, P17.VDD)
	NET_C(GND, P10.VSS, P13.VSS, P15.VSS, P16.VSS, P17.VSS)

	RES(R7, RES_K(2.7))
	RES(R8, RES_K(6.8))
	RES(R9, RES_K(18))
	RES(R10, RES_K(5.6))
	RES(R11, RES_K(180))
	RES(R12, RES_K(10))
	RES(R13, RES_K(68))
	RES(R14, RES_K(12))
	RES(R15, RES_K(1))
	RES(SPK1, 60)

	CAP(C2, CAP_U(4.7))
	CAP(C5, CAP_N(100))
	CAP(C6, CAP_N(10))

	DIODE(D2, "1N4002")
	DIODE(D3, "1N4002")
	DIODE(D4, "1N4002")

	QBJT_EB(T2, "2N3904") // BC183 NPN
	QBJT_EB(T3, "2N3904") // BC183 NPN
	QBJT_EB(T4, "2N3904") // BC183 NPN

	// pin 10
	NET_C(P10.Q, R7.1, C2.1)
	NET_C(R7.2, VBATT)
	NET_C(C2.2, T2.B, R11.1, D2.K, D3.K)
	NET_C(GND, R11.2, D3.A)
	NET_C(VBATT, T2.C)
	NET_C(T2.E, D4.A, R10.1)

	// pin 13
	NET_C(P13.Q, R8.1, R9.1)
	NET_C(R8.2, VBATT)
	NET_C(R9.2, C5.1)
	NET_C(C5.2, D4.K, D2.A)

	// pin 15-17
	NET_C(P15.Q, R13.1)
	NET_C(P16.Q, R14.1)
	NET_C(P17.Q, R15.1)

	// output
	NET_C(VBATT, T3.C, T4.C)
	NET_C(T3.B, R10.2, R13.2, R14.2, R15.2, C6.1)
	NET_C(C6.2, GND)
	NET_C(T3.E, T4.B, R12.1)
	NET_C(T4.E, R12.2, SPK1.1)
	NET_C(SPK1.2, GND)
}
