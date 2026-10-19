	if(islike(param, "/netstatus.ps3"))
	{
		// /netstatus.ps3          toggle network access in registry
		// /netstatus.ps3?1        enable network access in registry
		// /netstatus.ps3?enable   enable network access in registry
		// /netstatus.ps3?0        disable network access in registry
		// /netstatus.ps3?disable  disable network access in registry

		// /netstatus.ps3?ftp      ftp is running?
		// /netstatus.ps3?netsrv   netsrv is running?
		// /netstatus.ps3?ps3mapi  ps3mapi is running?
		// /netstatus.ps3?artemis  artemis is running?

		// /netstatus.ps3?stop-ftp      stop ftp server
		// /netstatus.ps3?stop-netsrv   stop net server
		// /netstatus.ps3?stop-ps3mapi  stop ps3mapi server
		// /netstatus.ps3?stop-artemis  stop artemis engine
		// /netstatus.ps3?stop          stop ps3mapi+net+ftp servers

		// /netstatus.ps3?start-ftp      start ftp server
		// /netstatus.ps3?start-netsrv   start net server
		// /netstatus.ps3?start-ps3mapi  start ps3mapi server
		// /netstatus.ps3?start-artemis  start artemis engine

		// /netstatus.ps3?reset-ftp      restart ftp server
		// /netstatus.ps3?reset-netsrv   restart net server
		// /netstatus.ps3?reset-ps3mapi  restart ps3mapi server
		// /netstatus.ps3?reset-artemis  restart artemis engine

		// /netstatus.ps3?ip=<address>
		// /netstatus.ps3?mask=<ip-mask>
		// /netstatus.ps3?route=<address>
		// /netstatus.ps3?dns1=<address>
		// /netstatus.ps3?dns2=<address>

		s32 status = 0; const char *label = NULL, *params = param + 15; xnet()->GetSettingNet_enable(&status);

		if(*params == 'f') {label = params, status = ftp_working;} else //ftp
		#ifdef PS3NET_SERVER
		if(*params == 'n') {label = params, status = net_working;} else //netsrv
		#endif
		#ifdef PS3MAPI
		if(*params == 'p') {label = params, status = ps3mapi_working;} else //ps3mapi
		#endif
		#ifdef ARTEMIS_PRX
		if(*params == 'a') {label = params, status = artemis_working;} else //artemis
		#endif
		if(*params == 's' || (params[0] == 'r' && params[1] == 'e')) // start / stop / reset / restart
		{
			const char *dash = strchr(params, '-');
			const char *service = dash ? (dash + 1) : NULL; // start-***, stop-***, reset-***, restart-***

			if(params[2] == 'o' || *params == 'r') // stop || reset || restart
			{
				if( !service || (*service == 'f')) {label = service, ftp_working = status = 0; sys_net_abort_socket(active_socket[0], SYS_NET_ABORT_STRICT_CHECK); sclose(&active_socket[0]);} //ftp
				#ifdef PS3MAPI
				if( !service || (*service == 'p')) {label = service, ps3mapi_working = status = 0; sys_net_abort_socket(active_socket[2], SYS_NET_ABORT_STRICT_CHECK); sclose(&active_socket[2]);} //ps3mapi
				#endif
				#ifdef PS3NET_SERVER
				if( !service || (*service == 'n')) {label = service, net_working = status = 0; sys_net_abort_socket(active_socket[3], SYS_NET_ABORT_STRICT_CHECK); sclose(&active_socket[3]);} //netsrv
				#endif
				#ifdef ARTEMIS_PRX
				if( !service || (*service == 'a')) {label = service, artemis_working = status = 0;} //artemis
				#endif
				if(*params == 'r') sys_ppu_thread_sleep(2);
			}
			if(params[2] == 'a' || *params == 'r') // start || reset || restart
			{
				if(!service || *service == 'f')
					{label = service, status = 1; if(!ftp_working) sys_ppu_thread_create(&thread_id_ftpd, ftpd_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_FTP_SERVER, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_FTP);} // start ftp daemon immediately
				#ifdef PS3MAPI
				if(!service || *service == 'p')
					{label = service, status = 1; if(!ps3mapi_working) sys_ppu_thread_create(&thread_id_ps3mapi, ps3mapi_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_PS3MAPI_SVR, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_PS3MAPI);}
				#endif
				#ifdef PS3NET_SERVER
				if(!service || *service == 'n')
					{label = service, status = 1; if(!net_working) sys_ppu_thread_create(&thread_id_netsvr, netsvrd_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_NET_SERVER, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_NETSVR);}
				#endif
				#ifdef ARTEMIS_PRX
				if(!service || *service == 'a')
					{label = service, status = 1; start_artemis();}
				#endif
			}
		}
		else if(*params == 'i' && params[2] == '=') // /netstatus.ps3?ip=<address>
		{
			xnet()->SetSettingNet_ipAddress((address *)&params[3]);
		}
		else if(*params == 'm' && params[4] == '=') // /netstatus.ps3?mask=<ip-mask>
		{
			xnet()->SetSettingNet_netmask((address *)&params[5]);
		}
		else if(*params == 'r' && params[5] == '=') // /netstatus.ps3?route=<address>
		{
			xnet()->SetSettingNet_defaultRoute((address *)&params[6]);
		}
		else if(*params == 'd' && params[4] == '=') // /netstatus.ps3?dns1=<address>
		{
			if(params[3] == '2')
				xnet()->SetSettingNet_secondaryDns((address *)&params[5]);
			else
				xnet()->SetSettingNet_primaryDns((address *)&params[5]);
		}
		else
		{
			if(*params == 0) ; else // query status
			if(!param[14]) {status ^= 1; xnet()->SetSettingNet_enable(status);} else
			if( param[15] & 1) xnet()->SetSettingNet_enable(1); else //enable
			if(~param[15] & 1) xnet()->SetSettingNet_enable(0);      //disable

			xnet()->GetSettingNet_enable(&status);
		}

		sprintf(param, "%s : %s", label ? to_upper((char*)label) : "Network", status ? STR_ENABLED : STR_DISABLED);

		#ifdef WM_REQUEST
		if(!wm_request)
		#endif
		{
			if(!mc) keep_alive = http_response(conn_s, header, "/netstatus.ps3", CODE_HTTP_OK, param);
		}

		show_msg(param);

		goto exit_handleclient_www;
	}
