#define SC_SYS_CONTROL_LED				(386)

#define GREEN			1
#define RED				2
#define YELLOW			2 //RED+GREEN (RED alias due green is already on)

#define OFF				0
#define ON				1
#define BLINK_FAST		2
#define BLINK_SLOW		3

static void led(u64 color, u64 mode)
{
	if(color == GREEN && mode == ON)
	{
		{ system_call_2(SC_SYS_CONTROL_LED, GREEN, ON); }
		if(is_qcfw)
			{ system_call_2(SC_SYS_CONTROL_LED, RED, ON); } // Turn ON red (RED+GREEN = YELLOW)
		else		
			{ system_call_2(SC_SYS_CONTROL_LED, RED, OFF); } // Turn OFF red (GREEN only)
	}
	else
		{system_call_2(SC_SYS_CONTROL_LED, color, mode);}
}
