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

document.onkeydown = function(evt) {
	evt = evt || window.event;
	if (evt.keyCode == 27) {
		if(document.activeElement.className != 'bs')
		{
			var divs = document.getElementsByTagName('div');
			for (var i = 0; i < divs.length; i++) {
				if(divs[i].style.maxHeight=='500px') divs[i].style.maxHeight='0px';
			}
			var bs = document.getElementsByClassName('bs'); 
			if(bs.length) bs[0].focus();
		}
		else
			window.scrollTo({ top: 0, behavior: 'smooth' });
	}
};
