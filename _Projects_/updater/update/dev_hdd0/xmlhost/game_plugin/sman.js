var dots=' <span class="dots"><span class="d1">.</span> <span class="d2">.</span> <span class="d3">.</span></span>';

function dot(){document.write(dots);}
function $(id){return document.getElementById(id);}
function s(id){return document.getElementById(id).style;}
function l(id,txt){
	txt=txt.replace('[','').replace(']','');$(id).innerHTML=txt;
	if(id=='refresh')$('msg1').innerHTML=txt+' XML'+dots;
	if(id=='msg1'||id=='msg2')return;
	if(txt.length>9){s(id).fontSize='16px';s(id).position ='relative';s(id).top ='4px';s(id).left ='-8px';}
}

var getAbsPosition = function(el){
	var el2 = el;
	var curleft = 0;
	if (document.getElementById || document.all) {
			curleft += el.offsetLeft-el.scrollLeft;
			el = el.offsetParent;
	} else if (document.layers) {
		curleft += el.x;
	}
	return curleft;
};

function resizeEvent()
{
	var cpursx = $("cpursx");
	cpursx.style.display = 'block';
	cpursx.style.display = (getAbsPosition(cpursx) < 650) ? 'none' : 'block';
	if(navigator.userAgent.indexOf("PLAYSTATION")!=-1) document.getElementsByClassName("b_cflow")[0].style.display='none';
};

function home()
{
	$('rhtm').style.display='block';
	if(typeof hurl !== 'undefined')
		self.location = hurl;
	else
		self.location='/index.ps3';	
}
