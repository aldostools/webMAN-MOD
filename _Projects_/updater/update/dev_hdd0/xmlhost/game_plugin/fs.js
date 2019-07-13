function t2lnks(){
	var txt=document.body;
	var url = /(\b(https?|ftp|file):\/\/[-A-Z0-9+&@#\/%?=~_|!:,.;]*[-A-Z0-9+&@#\/%=~_|])/ig;
	var www =/(^|[^\/#])(www\.[\S]+(\b|$))/gim;
	var dev =/(\/\b(dev_+.*))/ig;
	txt.innerHTML= "<a href='/'><b>webMAN MOD</b> 1.47.25<a><HR>" +
				  txt.innerHTML
				 .replace(url, "<a href='$1'>$1</a>")
				 .replace(www, '$1<a target="_blank" href="http://$2">$2</a>')
				 .replace(dev, '<a class=\42w\42 href=\42$1\42>$1</a>')+

	// Style links
	"<style>a{font-family:courier;text-decoration:none;color:#dddddd;}a:hover{color:#ffffff;}</style>"+

	// Right-click menu
	"<div id='mnu' style='position:fixed;width:180px;background:#333;z-index:9;display:none;padding:5px;box-shadow:3px 3px 6px #222;opacity:0.96'>" +
	"<a id='m0'>Install PKG</a>"+
	"<a id='m1'>Mount</a>"+
	"<a id='m2'>Open<br></a>" +
	"<hr>" +
	"<a id='m6'>Open Location<br></a>" +
	"<hr>" +
	"<a id='m3'>Delete<br></a>" +
	"<a id='m7'>Rename<br></a>" +
	"<a id='md'>MD5 Hash<br></a>"+
	"<hr>" +
	"<a id='m4'>Cut<br></a>" +
	"<a id='m5'>Copy<br></a>" +
	"<hr>" +
	"<a id='m8'>Copy To</a><br>" +
	"<a id='m9'>Copy & overwrite<br></a>"+
	"</div>"+
	"<script src='/dev_hdd0/xmlhost/game_plugin/common.js'></script><hr>"+
	" <input type='button' value=' &#9664;  ' onclick='location.href=\42javascript:history.back();\42;'>"+
	"</body></html>";
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

window.addEventListener('contextmenu',function(e){

	if(s)s.color='#ccc';
	t=e.target,s=t.style,c=t.className,m=mnu.style;if(c=='gi'){p=t.parentNode.pathname}else{p=t.pathname}if(typeof(p)!='string')return;p=p.replace('/mount.ps3','');
	if(c=='w'||c=='d'||c=='gi'||t.parentNode.className=='gn'){
		e.preventDefault();
		s.color='#fff',b='block',n='none';
		m.display=b;
		m.left=(e.clientX+12)+'px';
		y=e.clientY;w=window.innerHeight;m.top=(((y+220)<w)?(y+12):(w-220))+'px';
		y=(p.toLowerCase().indexOf('.pkg')>0);w=(p.toLowerCase().indexOf('.iso')>0);
		m0.href='/install.ps3'+p;m0.style.display=y?b:n;
		m1.href='/mount.ps3'+p;m1.style.display=!y&&(w||c=='d'||p.indexOf('/GAME')>0||p.indexOf('ISO/')>0)?b:n;
		m2.href=p;m2.text=(w||c=='w')?'Download':'Open';
		m3.href='/delete.ps3'+p;
		m4.href='/cut.ps3'+p;
		m5.href='/cpy.ps3'+p;
		m6.href=p.replace(/[^/]*$/, '');
		m7.href='javascript:rn(\"'+p+'\")';m7.style.display=(p.substring(0,5)=='/dev_')?b:n;
		m8.href='/copy.ps3'+p; m8.text = 'Copy to ' + ((p.indexOf('/dev_hdd')==0) ? "usb" : "hdd0");
		m9.href='/copy_ps3'+p;
		md.href='/md5.ps3'+p;
		y=p.indexOf('.ps3');if(y>0)p=p.substring(y+4);url=window.location.href;
	}
},false);

// Clear menu
window.onclick=function(e){if(m)m.display='none';t=e.target;if(t.id.indexOf('im')==0||(typeof(t.href)=='string'&&t.href.indexOf('.ps3')>0&&t.href.indexOf('prompt')<0&&t.href.indexOf('#Top')<0))wmsg.style.display='block';}
