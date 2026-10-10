function t2lnks(){
	var txt=document.body;
	var url = /(\b(https?|ftp|file):\/\/[-A-Z0-9+&@#\/%?=~_|!:,.;]*[-A-Z0-9+&@#\/%=~_|])/ig;
	var www =/(^|[^\/#])(www\.[\S]+(\b|$))/gim;
	var dev1 =/(\/\b(dev_+.*\t)([,0-9]+))/ig;
	var dev2 =/(\/\b(dev_+.*))/ig;
	var htab = txt.innerHTML.indexOf("\t") > 0;
	var count = txt.innerHTML.split("\n").length - 1;
	txt.innerHTML = "<a href='/'><b>webMAN MOD</b> 1.47.50</a><HR>"+
					"<style>td+td{width:100px;}</style><table>" +
				  txt.innerHTML
				 .replace(url, "<tr><td><a href='$1'>$1</td></tr>")
				 .replace(www, '<tr><td>$1<a target="_blank" href="http://$2">$2</a></td></tr>')
				 .replace(htab ? dev1 : dev2, htab? '<tr><td><a class=\42w\42 href=\42/$2\42>/$2</a></td><td align=right>$3</td></tr>' :
													'<tr><td><a class=\42w\42 href=\42$1\42>$1</a></td></tr>')
				+
	// Style links
	"</table><style>a{font-family:courier;text-decoration:none;color:#dddddd;}a:hover{color:#ffffff;}</style>" +

	// Right-click menu
	"<div id='mnu' style='position:fixed;width:180px;background:#333;z-index:9;display:none;padding:5px;box-shadow:3px 3px 6px #222;opacity:0.96'>" +
	"<a id='m0'>Install PKG</a>" +
	"<a id='m1'>Mount</a>" +
	"<a id='m2'>Open<br></a>" +
	"<hr>" +
	"<a id='v2'>Hex View<br></a>" +
	"<a id='m6'>Open Location<br></a>" +
	"<hr>" +
	"<a id='m3'>Delete<br></a>" +
	"<a id='m7'>Rename<br></a>" +
	"<a id='dm'>Truncate</a><br>"+
	"<a id='md'>MD5 Hash</a><br>" +
	"<hr>" +
	"<a id='m4'>Cut<br></a>" +
	"<a id='m5'>Copy<br></a>" +
	"<hr>" +
	"<a id='mz'>Zip Folder<br></a>"+
	"<a id='m8'>Copy To</a><br>" +
	"<a id='m9'>Copy & overwrite<br></a>" +
	"</div>" +
	"<script src='/dev_hdd0/xmlhost/game_plugin/common.js'></script><hr>" +
	" <input type='button' value=' &#9664;  ' onclick='location.href=\42javascript:history.back();\42;'> &nbsp; "+count+" item(s) found <span id='size'></span>"+
	"</body></html>";

	try{
		var size = 0;
		var rows = document.getElementsByTagName('tr');
		for(i=0;i<rows.length;i++){
			cols =rows[i].getElementsByTagName('td');
			size+=parseFloat(cols[1].innerHTML.replace(/,/g,""));
		}
		document.getElementById('size').innerHTML="&bull; "+(size/0x100000).toFixed(1).toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",")+" MB / "+size.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",")+" bytes";
	}
	finally{};
}

function tg(b,m,x,c){
	var i,p,o,h,l=document.querySelectorAll('.d,.w'),s=m.length,n=1;
	for(i=1;i<l.length;i++){
		o=l[i];
		h=o.href;p=h.indexOf('/cpy.ps3');if(p>0){n=0;s=8;bCpy.value='Copy';}
		if(p<1){p=h.indexOf('/cut.ps3');if(p>0){n=0;s=8;bCut.value='Cut';}}
		if(p<1){p=h.indexOf('/delete.ps3');if(p>0){n=0;s=11;bDel.value='Delete';}}
		if(p>0){o.href=h.substring(p+s,h.length);o.style.color='#ccc';}
		else{p=h.indexOf('/',8);o.href=m+h.substring(p,h.length);o.style.color=c;}
	}
	if(n)b.value=(b.value == x)?x+' Enabled':x;
}

// F2 = rename/move item pointed with mouse
document.addEventListener('keyup',ku,false);

function rn(f){
	if(f.substring(0,5)=='/dev_'){
		f=unescape(f);
		t=prompt('Rename to:',f);
		if(t&&t!=f)self.location='/rename.ps3'+f+'|'+escape(t)
	}
}
function ku(e){
	e=e||window.event;
	if(e.keyCode==113){try{a=document.querySelectorAll('a:hover')[0].pathname.replace('/mount.ps3','');rn(a);}catch(err){}}
}


var s,m;

document.oncopy = function(e){
	e.preventDefault();
	try{a=document.querySelectorAll('a:hover')[0].href;}catch(err){}
	var clipboard=e.clipboardData;
	clipboard.setData("text/plain",a);
};

function contextMenu(e){
	if(s)s.color='#ccc';
	t=e.target,s=t.style,c=t.className,m=mnu.style;if(c=='gi'){p=t.parentNode.pathname}else{p=t.pathname}if(typeof(p)!='string')return;p=p.replace('/mount.ps3','');
	if(c=='w'||c=='d'||c=='gi'||t.parentNode.className=='gn'){
		e.preventDefault();
		s.color='#fff',b='block',n='none';
		m.display=b;
		m.left=(e.clientX+12)+'px';pp=p.toLowerCase();
		y=(pp.indexOf('.pkg')>0);w=(pp.indexOf('.iso')>0);
		img=(pp.indexOf('.png')>0)||(pp.indexOf('.jpg')>0)||(pp.indexOf('.gif')>0);
		m0.href='/install.ps3'+p;m0.style.display=y?b:n;
		m1.href='/mount.ps3'+p;m1.style.display=!img&&!y&&(w||c=='d'||p.indexOf('/GAME')>0||p.indexOf('ISO/')>0||p.indexOf('ISO_')>0||p.indexOf('/ROMS')>0)?b:n;
		m2.href=p;m2.text=(w||c=='w')?'Download':'Open';
		v2.href='/view.ps3'+p;v2.style.display=(c!='d')?b:n;
		exts=/(\.txt|\.ini|\.log|\.ncl|\.sfx|\.xml|\.cfg|\.cnf|\.his|\.hip|\.bup|\.js|\.css|\.htm|\.bat|\.yaml|\.conf)$/i;if(exts.exec(p)){v2.href='/edit.ps3'+p;v2.text='Edit';}
		m3.href='/delete.ps3'+p;
		m4.href='/cut.ps3'+p;
		m5.href='/cpy.ps3'+p;
		m6.href=p.replace(/[^/]*$/, '');
		m7.href='javascript:rn(\"'+p+'\")';m7.style.display=(p.substring(0,5)=='/dev_')?b:n;
		m8.href='/copy.ps3'+p; m8.text = 'Copy to ' + ((p.indexOf('/dev_hdd')==0) ? "usb" : "hdd0");
		m9.href='/copy_ps3'+p;
		mz.href=((c=='d')?'/dozip.ps3':'/unzip.ps3')+p;
		mz.text=((c=='d')?'Zip Folder':'Unzip');exts=/(\.zip|\.rar|\.7z|\.bz2|\.tar|\.tgz)$/i;
		mz.style.display=(c=='d')?b:(exts.exec(p)?b:n);
		md.href=((c=='d')?'/stat.ps3':'/md5.ps3')+p;
		md.text=((c=='d')?'Folder Size':'MD5 Hash');
		dm.href=((c=='d')&&!p.includes('/USRDIR/')?'/chmod.ps3':'/trunc.ps3')+p;
		dm.text=((c=='d')&&!p.includes('/USRDIR/')?'Fix Permissions':'Truncate');
		y=p.indexOf('.ps3');if(y>0)p=p.substring(y+4);url=window.location.href;
		y=e.clientY;w=window.innerHeight;m.top=(((y+mnu.clientHeight)<w)?(y+12):(w-mnu.clientHeight))+'px';
	}
}

window.addEventListener('contextmenu',contextMenu,false);

// Clear menu
var tapHold, tapEvent, tapEventFired = false;
window.onclick=function(e){if(tapEventFired){tapEvent.preventDefault();tapEvent.stopPropagation();return false;}if(m)m.display='none';wmsg.style.display='none';t=e.target;if(t.id.indexOf('im')==0||(typeof(t.href)=='string'&&t.href.indexOf('.ps3')>0&&t.href.indexOf('view.ps3')<0&&t.href.indexOf('.png')<0&&t.href.indexOf('.jpg')<0&&t.href.indexOf('prompt')<0&&t.href.indexOf('#Top')<0))wmsg.style.display='block';}

document.addEventListener('mousedown', function(event){
	event.preventDefault();
	tapEvent = event;
	tapEventFired = false;
	tapHold = setTimeout(function(e){
		tapEventFired = true;
		tapEvent.stopPropagation();
		contextMenu(tapEvent);
		clearTimeout(tapHold);
	}, 2000);
},false);

document.addEventListener('mouseup', function(event){
	clearTimeout(tapHold);
	if(tapEventFired) {event.preventDefault();event.stopPropagation();return false;}
},false);
