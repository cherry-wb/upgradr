// ==UserScript==
// @name		All in One Lightbox
// @namespace		http://shiftingpixel.com
// @description		Adds lightbox functionality to links that point to images. Use left and right arrow keys to cycle through images on page.
// @include			*
// ==/UserScript==

(function(){


function addEvent( obj, type, fn ) {
	if ( obj.attachEvent ) {
		obj["e"+type+fn] = fn;
		obj[type+fn] = function() { obj["e"+type+fn]( window.event ); return false;}
		obj.attachEvent( "on"+type, obj[type+fn] );
	} else
		obj.addEventListener( type, fn, false );
}

function removeEvent( obj, type, fn ) {
	if ( obj.detachEvent ) {
		obj.detachEvent( "on"+type, obj[type+fn] );
		obj[type+fn] = null;
	} else
		obj.removeEventListener( type, fn, false );
}

var greasedLightbox = {
	// version
	// used to check for updates
	version : 0.14,
	
	/*
	searchDefs stores function names and regular expressions used to find and execute functions
	for image links within the page. these are executed in the order that they appear.
	
	name					= self-explanitory
	includeRegExp			= regular expression that window.location.href needs to match
	linkRegExp				= regular expression that link must match
	excludeLinkRegExp		= regular expression that link must not match
	findImageRegExp			= regular expression that image must match for replaceString
	replaceString			= replace string used with findRegExp (required with findImageRegExp, optional with linkRegExp)
	showFunction			= function that is called when link is clicked
	
	for links that reveal larger images use name, includeRegExp, linkRegExp, and showFunction (replaceString and excludeLinkRegExp are optional)
	for images that reveal larger images use name, includeRegExp, linkRegExp, findImageRegExp, replaceString, and showFunction (excludeLinkRegExp is optional)
	*/
	searchDefs : [
		// wikipedia (needs to come before 'show')
		{
			name				: 'wikipedia',
			includeRegExp		: /^http(s)?:\/\/(.*?\.)?wikipedia.org/i,
			linkRegExp			: /(.*?)\/Image:(.*)\.(jp(e?)g|gif|png|svg)$/i,
			findImageRegExp		: /(.*?)\/thumb\/(.*?)\.(jp(e?)g|gif|png)$/i,
			replaceString		: '$1/$2.$3',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'wikipedia'); return false; }
		}, // end wikipedia

		// imagesocket (needs to come before 'show')
		{
			name			: 'imagesocket',
			includeRegExp		: /./, // used on every page
			linkRegExp		: /^(http(s)?:\/\/(.*?\.)?imagesocket\.com)\/view\/(.*?\.(jp(e?)g|gif|png))$/i,
			replaceString		: '$1/images/$4',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'imagesocket'); return false; }
		}, // end imagesocket
		
		// fileanchor ()
	
		// regular links to images
		{
			name			: 'show',
			includeRegExp		: /./, // used on every page
			linkRegExp		: /(.*?)\.(jp(e?)g|gif|png)$/i,
			excludeLinkRegExp	: /imageshack.us[^\?]*\?/i,
			showFunction		: function(e) { greasedLightbox.show(e); return false; }
		}, // end regular links to images

		// search engine images (google, yahoo, ask jeeves, blingo)
		{
			name			: 'search',
			includeRegExp		: /^http(s)?:\/\/(.*?\.)?(images\.google\..*|search\.yahoo\.com|blingo\.com\/images)/i, 
						//http://tbn0.google.com/images?q=tbn:jtdwDU00VPXkGM:http://web2.airmail.net/uthman/autop_tools_pix/autopsy_tools.jpg
			linkRegExp		: /(.*?)(im(age|g)(ur(i|l)|src))=(http(s?):\/\/)?(.*?)&(.*)/i,
			//linkRegExp		: /http:\/\/(.*?):http:\/\/(.*?)/i,
			replaceString		: 'http$7://$8',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'search'); return false; }
		}, // end search engine images
		
		// flickr
		{
			name			: 'flickr',
			includeRegExp		: /^http(s)?:\/\/(.*?\.)?flickr.com/i,
			linkRegExp		: /\/photos\/[^\/]+\/[0-9]+/i,
			findImageRegExp		: /_[tsm]/i,
			replaceString		: '',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'flickr'); return false; }
		}, // end flickr
		
		// facebook
		{
			name			: 'facebook',
			includeRegExp		: /^http(s)?:\/\/(.*?\.)?(the)?facebook.com/i,
			linkRegExp		: /photo\.php\?pid=[0-9]+/i,
			findImageRegExp		: /s([0-9]+.*?)\.jpg/i,
			replaceString		: 'n$1.jpg',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'facebook'); return false; }
		}, // end facebook
		
		// myspace
		{
			name			: 'myspace',
			includeRegExp		: /^http(s)?:\/\/(.*?\.)?myspace.com/i,
			linkRegExp		: /imageID=[0-9]+/i,
			findImageRegExp		: /([0-9]+_)m\.jpg/i,
			replaceString		: '$1l.jpg',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'myspace'); return false; }
		}, // end myspace
		
		// deviantart
		{
			name			: 'deviantart',
			includeRegExp		: /^http(s)?:\/\/(.*?\.)?deviantart.com/i,
			linkRegExp		: /deviantart\.com\/(deviation|print)\/[0-9]+/i,
			findImageRegExp		: /http(s)?:\/\/(.*?)\.deviantart\.com\/([^\/]*)\/([^\/]*)\/(.*?)\.(jp(e?)g|gif|png)/i,
			replaceString		: 'http$1://$2.deviantart.com/$3/300W/$5.jpg',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'deviantart'); return false; }
		}, // end deviantart
		
		// subvariance
		{
			name			: 'subvariance',
			includeRegExp		: /^http(s)?:\/\/(.*?\.)?subvariance.com/i,
			linkRegExp		: /\?f=[0-9]+&(amp;)?id=[0-9]+/i,
			findImageRegExp		: /\/items\/thumbs\/(.*?)\.jpg/i,
			replaceString		: '/items/med/$1.jpg',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'subvariance'); return false; }
		}, // end subvariance
		
		// gmail
		{
			name			: 'gmail',
			includeRegExp		: /^http(s)?:\/\/mail\.google\..*/i,
			linkRegExp		: /^(\/mail\/\?view=att&(amp;)?disp=)inline/i,
			replaceString		: 'http://' + window.location.host + '$1emb',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'gmail'); return false; }
		}, // end gmail
		
		// textamerica
		{
			name			: 'textamerica',
			includeRegExp		: /^http(s)?:\/\/(.*?\.)?textamerica.com/i,
			linkRegExp		: /\?r=[0-9]+$/i,
			findImageRegExp		: /user\.images\.x\/(.*?\/.*?)\/(.*?)\/(.*?)\.jpg(.*)$/i,
			replaceString		: 'user.images.x/$1\/$3.jpg',
			showFunction		: function(e) { greasedLightbox.showFrom(e, 'textamerica'); return false; }
		} // end textamerica

	], // end searchDefs[]
	
	// useLinkForShow()
	useLinkForShow : function(searchDef) {
		if (searchDef.findImageRegExp) {
			return false;
		} else {
			return true;
		}
	},
	
	// showFrom()
	// generic helper function that calls show() with the correct parameters
	showFrom : function(e, showName) {
		if( typeof e == 'undefined' ) e = window.event;
		var link;
		if (e.srcElement) {
			link							= e.srcElement;
			if(link.tagName == 'IMG') link = link.parentNode;	//fix for IE
		} else {
			link							= e;
		}
		var address							= unescape(unescape(greasedLightbox.getAddress(link)));
		var img								= greasedLightbox.getImageToShow(link, address, showName);
		greasedLightbox.show(e, img, address);
	}, // end showFromLink()
	
	// getImageToShow()
	getImageToShow : function(link, address, showName) {
		var searchDef						= greasedLightbox.getRegExpObj(greasedLightbox.searchDefs, showName);
		
		if (greasedLightbox.useLinkForShow(searchDef)) {
			address							= unescape(unescape(address));
			if (searchDef['replaceString']) {
				var img						= address.replace(searchDef['linkRegExp'], searchDef['replaceString']);
			} else {
				var img						= address.match(searchDef['linkRegExp'])[0];
			}
		} else {
			var img							= greasedLightbox.containsThumb(link, greasedLightbox.getRegExpObj(greasedLightbox.searchDefs, showName), true);
		}
		return img;
	}, // end getImageToShow()
	
	// getRegExpObj()
	// returns the requested regular expression object from the regExp array
	getRegExpObj : function(regExpObject, showName) {
		var rExObj;
		
		for (var i = 0; i < regExpObject.length; i++) {
			rExObj							= regExpObject[i];
			if (rExObj['name'] == showName) {
				return rExObj;
			}
		}
	}, // end getRegExpObj()
	
	// containsThumb()
	containsThumb : function(elem, rExObj, verbose) {
		for(var i=0; i< elem.childNodes.length; i++) {
			if(elem.childNodes[i].nodeName == 'IMG') {
				if(rExObj['findImageRegExp'].test(elem.childNodes[i].getAttribute('src'))) {
					if(!verbose) return true;
					return elem.childNodes[i].getAttribute('src').replace(rExObj['findImageRegExp'], rExObj['replaceString']);
				}
			}
		}
		return false;
	}, // end containsThumb()
	
	// getAddress()
	// extracts an address out of a linkObj
	getAddress : function(linkObj) {
        	var address                            = linkObj.getAttribute('href');
       
        	// for creammonkey users because Safari doesn't like stopping events even though it says it does...
        	if(/Safari/.test(navigator.userAgent)) {
        	    linkObj.onclick = function() { return false; };
        	}
        	return address;
    	}, // end getAddress()
	
	// getPageScroll()
	// Returns array with x,y page scroll values.
	// Core code from - quirksmode.org
	getPageScroll : function() {
		var xScroll, yScroll;

		if (self.pageYOffset) {
			yScroll 						= self.pageYOffset;
		} else if (document.documentElement && document.documentElement.scrollTop){	 // Explorer 6 Strict
			yScroll 						= document.documentElement.scrollTop;
		} else if (document.body) {// all other Explorers
			yScroll 						= document.body.scrollTop;
		}
		
		if (self.pageXOffset) {
			xScroll 						= self.pageXOffset;
		} else if (document.documentElement && document.documentElement.scrollLeft){	 // Explorer 6 Strict
			xScroll 						= document.documentElement.scrollLeft;
		} else if (document.body) {// all other Explorers
			xScroll 						= document.body.scrollLeft;
		}
	
		arrayPageScroll 					= new Array(xScroll,yScroll) 
		return arrayPageScroll;
	}, // end getPageScroll()
	
	// getPageSize()
	// Returns array with page width, height and window width, height
	// Core code from - quirksmode.org
	// Edit for Firefox by pHaez
	getPageSize : function() {
		var xScroll, yScroll;
		
		if (window.innerHeight && window.scrollMaxY) {	
			xScroll							= document.body.scrollWidth;
			yScroll							= window.innerHeight + window.scrollMaxY;
		} else if (document.body.scrollHeight > document.body.offsetHeight){ // all but Explorer Mac
			xScroll							= document.body.scrollWidth;
			yScroll							= document.body.scrollHeight;
		} else { // Explorer Mac...would also work in Explorer 6 Strict, Mozilla and Safari
			xScroll							= document.body.offsetWidth;
			yScroll							= document.body.offsetHeight;
		}
		
		var windowWidth, windowHeight;
		if (self.innerHeight) {	// all except Explorer
			windowWidth						= self.innerWidth;
			windowHeight					= self.innerHeight;
		} else if (document.documentElement && document.documentElement.clientHeight) { // Explorer 6 Strict Mode
			windowWidth						= document.documentElement.clientWidth;
			windowHeight					= document.documentElement.clientHeight;
		} else if (document.body) { // other Explorers
			windowWidth						= document.body.clientWidth;
			windowHeight					= document.body.clientHeight;
		}	
		
		// for small pages with total height less then height of the viewport
		if(yScroll < windowHeight){
			pageHeight						= windowHeight;
		} else { 
			pageHeight						= yScroll;
		}
	
		// for small pages with total width less then width of the viewport
		if(xScroll < windowWidth){	
			pageWidth						= windowWidth;
		} else {
			pageWidth						= xScroll;
		}
	
		arrayPageSize						= new Array(pageWidth,pageHeight,windowWidth,windowHeight) 
		return arrayPageSize;
	}, // end getPageSize()
	
	// center()
	// centers the object in the page
	center : function(objToCenter, arrayPageScroll, arrayPageSize) {
		var objTop						= arrayPageScroll[1] + ((arrayPageSize[3] - 35 - objToCenter.offsetHeight) / 2);
		var objLeft						= (arrayPageSize[0] - objToCenter.offsetWidth) / 2;
		
		objToCenter.style.top			= (objTop < 0) ? "0px" : objTop + "px";
		objToCenter.style.left			= (objLeft < 0) ? "0px" : objLeft + "px";
		
		// I don't know exactly why, but it doesn't always center it properly if this isn't done twice...		
		objToCenter.style.top			= (objTop < 0) ? "0px" : objTop + "px";
		objToCenter.style.left			= (objLeft < 0) ? "0px" : objLeft + "px";
	}, // end center()
	
	// currentAddress
	// address of the link that was clicked on. updated from show() and is used in noimage()
	currentAddress : null,
	
	// show()
	// Preloads images. Pleaces new image in lightbox then centers and displays.
	show : function(e, img, context) {
		if( typeof e == 'undefined' ) e = window.event;
		// let shift+click and ctrl+click (but not ctrl+shift+click) through without lightbox
		if ((e.shiftKey || e.ctrlKey) && !(e.shiftKey && e.ctrlKey)) return true;
			
		// if this is a real event stop the click and set the link, otherwise, just set the link
		if (e.srcElement) {
			var link						= e.srcElement;
			// if original lightbox js is in action, leave it be
			if (link.getAttribute('rel') == 'lightbox') return true;
			
			//e.stopPropagation();
			//e.preventDefault();
			e.cancelBubble = true;
		} else {
			var link						= e;
		}



		if (img == null || img == '') img	= link.getAttribute('href');
		greasedLightbox.currentAddress		= unescape(unescape(greasedLightbox.getAddress(link)));
		
		// make ctrl+shift+click follow link without lightbox
		if (e.shiftKey && e.ctrlKey) {
			window.location.href			= greasedLightbox.currentAddress;
			return true;
		}
		
		greasedLightbox.isShowing			= true;
		
		// get the caption from the title attribute of the link. if that doesn't exist, look for it in the title attribute of the image.
		capt								= link.getAttribute('title');
		
		if (capt == null || capt == '') {
			try {
				var imgObj					= link.firstChild;
				capt						= imgObj.getAttribute('title');
			} catch (e) { }
		}
		if (capt == null || capt == '') {
			try {
				var imgObj					= link.firstChild;
				capt						= imgObj.getAttribute('alt');
			} catch (e) { }
		}
		
		// prep objects
		var objOverlay						= document.getElementById('greasedLightboxOverlay');
		var objMenu							= document.getElementById('greasedLightboxMenu');
		var objLightbox						= document.getElementById('greasedLightbox');
		var objCaption						= document.getElementById('greasedLightboxCaption');
		var imgPreload						= document.getElementById('greasedLightboxPreload');
		var objImage						= document.getElementById('greasedLightboxImage');
		var objLoading						= document.getElementById('greasedLightboxLoading');
		
		var arrayPageSize					= greasedLightbox.getPageSize();
		var arrayPageScroll 				= greasedLightbox.getPageScroll();
	
		// set height of Overlay to take up whole page and show
		objOverlay.style.height				= (arrayPageSize[1] + 'px');
		objOverlay.style.display			= 'block';
		
		// show menu
		objMenu.style.display				= 'block';
		
		// center loader and error message
		objLoading.style.visibility			= 'hidden';
		objLoading.style.display			= 'block';
		greasedLightbox.center(objLoading, arrayPageScroll, arrayPageSize);
		objLoading.style.visibility			= 'visible';
		
		var imgPreload						= document.getElementById('greasedLightboxPreload');
		// preload image
		preloaderDone = function() {

			objImage.src					= img;
			objImage.removeAttribute('width');
			objImage.removeAttribute('height');
			greasedLightbox.aspectRatio		= null;
			
			if (capt) {
				objCaption.innerHTML		= capt;
			} else {
				objCaption.innerHTML		= img;
			}
			
			// dimensions
			// objCaption.innerHTML			= objCaption.innerHTML + '<br/><br/>(width: ' + objImage.width + 'px; height: ' + objImage.height + 'px;)';
			
			// add a link for context
			if (context) objCaption.innerHTML	= objCaption.innerHTML + '<br/><br/><a href="' + context + '">' + greasedLanguage[greasedLanguage.language][0].context + '</a>';
			
			// center lightbox and make sure that the top and left values are not negative
			// and the image placed outside the viewport
			objLightbox.style.visibility	= 'hidden';
			objLightbox.style.display		= 'block';
			
			greasedLightbox.aspectRatio		= objImage.height / objImage.width;
			
			// if image is larger than the screen
			if (objImage.height > arrayPageSize[3] - 70) {
					var newHeight			= arrayPageSize[3] - 70;
					var newWidth			= (objImage.width / objImage.height) * newHeight;
					objImage.height			= newHeight;
					objImage.width			= newWidth;
			}
			if (objImage.width > arrayPageSize[2] - 70) {
					var newWidth			= arrayPageSize[2] - 70;
					var newHeight			= greasedLightbox.aspectRatio * newWidth;
					objImage.height			= newHeight;
					objImage.width			= newWidth;
			}
			
			greasedLightbox.center(objLightbox, arrayPageScroll, arrayPageSize);
			objLoading.style.display		= 'none';
			objCaption.style.display		= 'block';
			objLightbox.style.visibility	= 'visible';
						
			// if it went bigger than the page
			if (objLightbox.offsetHeight > objOverlay.offsetHeight) objOverlay.style.height		= objLightbox.offsetHeight + 'px';
			if (objLightbox.offsetWidth > objOverlay.offsetwidth) objOverlay.style.width		= objLightbox.offsetWidth + 'px';
			
			// clean it up a bit for memory's sake
			removeEvent(imgPreload, 'load', preloaderDone);
			removeEvent(imgPreload, 'error', greasedLightbox.noImage);
			imgPreload.src					= '';
			return false;

		} // end preloaderDone()

		if (imgPreload.src != img) {
			addEvent(imgPreload, 'load', preloaderDone);
			addEvent(imgPreload, 'error', greasedLightbox.noImage);
			imgPreload.src					= img;
		} else {
			preloaderDone();
		}

		
		// hides flash movies that peek through the overlay
		var objects						= document.getElementsByTagName('object');
		for (i = 0; i != objects.length; i++) {
			objects[i].style.visibility	= 'hidden';
		}

		var embeds						= document.getElementsByTagName('embed');
		for (i = 0; i != embeds.length; i++) {
			embeds[i].style.visibility = 'hidden';
		}

		var iframes						= document.getElementsByTagName('iframe');
		for (i = 0; i != iframes.length; i++) {
			iframes[i].style.visibility = 'hidden';
		}
		if (greasedLightbox.allImageLinks.length > 1) {
			// initialize slideshow
			// set currentImagePosition
			findCurrentPosition : for(var i = 0; i < greasedLightbox.allImageLinks.length; i++) {
				if (greasedLightbox.allImageLinks[i]['link'] == link) {
					greasedLightbox.currentImagePosition	= i;
					break findCurrentPosition;
				}
			} // end for()
			
			// pre-fetch next image
			var imgPrefetch					= document.getElementById('greasedLightboxPrefetch');
			
			var nextImagePosition			= (greasedLightbox.currentImagePosition + greasedLightbox.lastMove) % greasedLightbox.allImageLinks.length;
			if (nextImagePosition < 0) nextImagePosition = greasedLightbox.allImageLinks.length - 1;
			
			var nextImage					= greasedLightbox.allImageLinks[nextImagePosition];
			var nextImageSrc				= greasedLightbox.getImageToShow(nextImage['link'], greasedLightbox.getAddress(nextImage['link']), nextImage['name']);
			imgPrefetch.src					= nextImageSrc;
		} // end if()
		
	}, // end show()
	
	// hide()
	// Stops the preloader in case it hasn't finished and then hides all of the lightbox components
	hide : function(e) {
		if( typeof e == 'undefined' ) e = window.event;
		//e.stopPropagation();
		//e.preventDefault();
		e.cancelBubble = true;
		
		greasedLightbox.isShowing			= false;
		//greasedLightbox.aspectRation		= null;
		
		// get objects
		var objPreloader					= document.getElementById('greasedLightboxPreload');
		var objLoading						= document.getElementById('greasedLightboxLoading');
		var objError						= document.getElementById('greasedLightboxError');
		var objOverlay						= document.getElementById('greasedLightboxOverlay');
		var objLightbox						= document.getElementById('greasedLightbox');
		var objMenu							= document.getElementById('greasedLightboxMenu');
		var imgPreload						= document.getElementById('greasedLightboxPreload');
		
		// stop preloader
		removeEvent(objPreloader, 'load', preloaderDone);
		removeEvent(imgPreload, 'error', greasedLightbox.noImage);
		imgPreload.src						= '';
		
		// show flash movies again
		var objects							= document.getElementsByTagName('object');
		for (i = 0; i != objects.length; i++) {
			objects[i].style.visibility		= 'visible';
		}

		var embeds							= document.getElementsByTagName('embed');
		for (i = 0; i != embeds.length; i++) {
			embeds[i].style.visibility		= 'visible';
		}

		var iframes							= document.getElementsByTagName('iframe');
		for (i = 0; i != iframes.length; i++) {
			iframes[i].style.visibility = 'visible';
		}

		// hide everything
		objLoading.style.display			= 'none';
		objError.style.display				= 'none';
		objOverlay.style.display			= 'none';
		objLightbox.style.display			= 'none';
		objMenu.style.display				= 'none';
		
	}, // end hide()

	aspectRatio: null,
        
	// resize()
	resize : function(e, resizeByAmount) {
		if( typeof e == 'undefined' ) e = window.event;
		//e.stopPropagation();
		//e.preventDefault();
		e.cancelBubble = true;
		
		// resize the image
		var objImage						= document.getElementById('greasedLightboxImage');
		var imgPreload						= document.getElementById('greasedLightboxPreload');
                
		if (resizeByAmount == 0) {
			objImage.removeAttribute('width');
			objImage.removeAttribute('height');
		} else {
			var newWidth					= objImage.width + (objImage.width * (resizeByAmount/100));
			var newHeight					= this.aspectRatio * newWidth;
			if (newWidth > 30 || newHeight > 30) {
				objImage.width				= newWidth;
				objImage.height				= newHeight;
			}
		}
		
		// re-center the lightbox
		var objLightbox						= document.getElementById('greasedLightbox');
		var arrayPageSize					= greasedLightbox.getPageSize();
		var arrayPageScroll 				= greasedLightbox.getPageScroll();
		greasedLightbox.center(objLightbox, arrayPageScroll, arrayPageSize);
	}, // end resize()
	
	// noImage()
	// Displays a nice error message when no image can be found.
	noImage : function(e) {
		if( typeof e == 'undefined' ) e = window.event;
		var objLoading						= document.getElementById('greasedLightboxLoading');
		var objError						= document.getElementById('greasedLightboxError');
		var objErrorContext					= document.getElementById('greasedLightboxErrorContext');
		var arrayPageSize					= greasedLightbox.getPageSize();
		var arrayPageScroll 				= greasedLightbox.getPageScroll();
		
		objError.style.visibility			= 'hidden';
		objError.style.display				= 'block';
		
		objErrorContext.innerHTML			= '<a href="' + greasedLightbox.currentAddress + '">' + greasedLanguage[greasedLanguage.language][0].context + '</a>';
		
		greasedLightbox.center(objError, arrayPageScroll, arrayPageSize);
		
		objLoading.style.display			= 'none';
		objError.style.visibility			= 'visible';
	}, // end noImage()
	
	// allImageLinks[]
	// An array of image links and their functions generated by init()
	// allImageLinks[i]['name']				= name of the searchDef that found this link
	// allImageLinks[i]['showFunction']		= function that gets added to the link onclick
	// allImageLinks[i]['link']				= value of link (is equivalent to event.srcElement on a click event)
	allImageLinks : new Array(0),
	
	// currentImagePosition
	// position of currently showed image in allImageLinks[]
	currentImagePosition : 0,
	
	// lastMove
	// keeps track of which arrow key user last used (1 = right, -1 = left)
	lastMove : 1,
	
	// isShowing
	// true if lightbox is currently showing. updated by show() and hide()
	isShowing : false,
    
	// handleKey(event)
	// handles keypress. If 'x' is pressed then it hides the lightbox. If a left or right arrow is pressed it cycles through images on a page
	handleKey : function(e) {
		if( typeof e == 'undefined' ) e = window.event;
		if (greasedLightbox.isShowing) {
			var keycode							= e.which;
			var key								= String.fromCharCode(keycode).toLowerCase();
			
			switch(key) {
				case 'x':
					greasedLightbox.hide(e);
					break;
				case '+':
					greasedLightbox.resize(e, 13);
					break;
				case '-':
					greasedLightbox.resize(e, -13);
					break;
				case '0':
					greasedLightbox.resize(e, 0);
					break;
				default:
					switch(e.keyCode) {
						// esc key
						case 27:
							greasedLightbox.hide(e);
							break;
							
						// left arrow
						case 37:		// firefox
						case 63234:		// safari
							greasedLightbox.moveSlide(e, -1);
							break;
							
						// right arrow
						case 39:		// firefox
						case 63235:		// safari
							greasedLightbox.moveSlide(e, 1);
							break;
							
					} // end switch(e.keyCode)
					break;
			} // end switch(key)
		} // end if
	}, // end getKey()
	
	// moveSlide()
	// loads another image from allImageLinks[]
	moveSlide : function(e, moveByAmount) {
		if (greasedLightbox.allImageLinks.length > 1) {
			if (greasedLightbox.currentImagePosition + moveByAmount == -1) greasedLightbox.currentImagePosition = greasedLightbox.allImageLinks.length;

			var newSlidePosition				= (greasedLightbox.currentImagePosition + moveByAmount) % greasedLightbox.allImageLinks.length;
			var slideToLoad						= greasedLightbox.allImageLinks[newSlidePosition];
			
			greasedLightbox.hide(e);
			slideToLoad['showFunction'](slideToLoad['link']);
			greasedLightbox.lastMove			= moveByAmount;
		} // end if
	}, // end moveSlide()
	
	// lightBulbOnIcon
	// used when an update is available
	lightBulbOnIcon :"url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAABGdBTUEAAK%2FINwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAKgSURBVDjLlZLrS1NxGMd90ZvovdEfEBEUEhZIb0xMjdyLIuyGkiHGUFKydFKKJiRegjIyFJRwojMxzfJSaVOYeTfxtpSNuZ1tXnY2z27nsss5334uWloG9uLD7%2FA7z%2FfzPPx4IgBE7ISl3qWyelUvu9JIueZqeOdUmcCMFDgcQ3fntjSK0j%2Frwx%2BcsesIZ3jbL1j6EbCPIej5DpE3QRIoBJ3LEFb74BjIxkbXVYNdrTixS8Ca3h%2Fy6pSTfloD0UcRjCS8BJGbRdA7QRgjd1pIfhruyeewKOMdm%2BrCw2GBV1tXKZh7SIEVoqAjpwVS0AlIvhBSkCGyeQRcPYDogO1DNixvrveFBa6ZCkuAmSe1OtJpFVLATkJboWCIAE3%2BGYngI6ENgnUK%2BhcxfFiw9fWRT%2BRWEWTHEeRmyPhaMvYCgu5ZEpgkbzCCgPszBNsr8NY8iF4Ky5WnpLDArs41%2BzYnSPdF8OYi0qEcTHc6mF45mJ4M2Ftl4C1lYPU34KerwFNTWKmO%2Fj2BfbiwghmvJuPawZsUsNVHgTPlEx6ANcjJeR9r5QfhWUqEJOlhbc%2BFoV42FBY4R0sPbPbKlz2LLeQB9aCbYkJhzpIFlkoDZ8zDRk0kRHYYrm8d0JYeEyyduUd37QH9pTBqvSOV9iy0wtmZ%2BVNAOm%2BHOeM92JtlYDQN0JYcD1BtmTf%2FWqRtbJ%2FyTxtUt9fXGhPBq5MhriVBtMYhoLkMQ1Ek5sqi3eb2O4l7buIvhlRPkmsfZ%2Fibax%2BiruosnpacQUFOOq7Fn5TUypJz%2F1zlnRQr5JSypRVKZRvq6htR%2FewlriTH03vV7ilQ5NwaHRgchM1GY3p6Bq%2BbmpEii9XtWzCgqkhLuXSBTUg4L8XFxUoXk2K57obirH0L%2FocfNQ8V8wE%2BuE0AAAAASUVORK5CYII%3D')",
	
	// showUpdate()
	// shows the user that an update is available
	showUpdate : function(latestVersion) {
		var titleLink	= document.getElementById('greasedLightboxTitleLink');
		titleLink.setAttribute('title', greasedLanguage[greasedLanguage.language][0].update + ' (v' + latestVersion + ')');
		titleLink.innerHTML				= titleLink.innerHTML + ' - ' + greasedLanguage[greasedLanguage.language][0].update + ' (v' + latestVersion + ')';
		var cssStr 						= '#greasedLightboxTitleLink { background-image: ' + greasedLightbox.lightBulbOnIcon + ' !important; }';
		
		var styleSheet					= document.getElementById('greasedLightboxCSS');
		var cssText						= document.createTextNode(cssStr);
		styleSheet.appendChild(cssText);
	},
	
	// init()
	// Function runs on window load, going through link tags looking for links to images.
	// These links receive onclick events that enable the lightbox display for their targets.
	// The function also inserts html markup at the top of the page which will be used as a
	// container for the overlay pattern and the inline image.
	init : function() {
		// initialize localization
		greasedLanguage.init();
		
		// set up list of searchDefs to use based on how includeRegExp matches window.location.href
		var currentURL						= window.location.href;
		var searchDefsToUse					= new Array();
		
		
		for(var i = 0; i < greasedLightbox.searchDefs.length; i++) {
			if(greasedLightbox.searchDefs[i]['includeRegExp'].test(currentURL)) searchDefsToUse.push(greasedLightbox.searchDefs[i]);
		}
		if(!searchDefsToUse.length) return;
		
		// set variables
		var links							= document.getElementsByTagName('a');
		var link							= null;
		var lightboxedLinksTotal			= 0;
		for(var i = 0; i < links.length; i++) {
			checkLink: if (link = unescape(links[i].getAttribute('href'))) {
				// check regularExpressions from searchDefsToUse
				for (var ii = 0; ii < searchDefsToUse.length; ii++) {
					// for links that reveal larger image's location
					if (!searchDefsToUse[ii]['findImageRegExp']) {
						if(searchDefsToUse[ii]['linkRegExp'].test(link)) {
							if (!searchDefsToUse[ii]['excludeLinkRegExp'] || !searchDefsToUse[ii]['excludeLinkRegExp'].test(link)) {
								addEvent(links[i], 'click', searchDefsToUse[ii]['showFunction']);
								this.allImageLinks[lightboxedLinksTotal]		= new Array(3);
								this.allImageLinks[lightboxedLinksTotal]['name']	= searchDefsToUse[ii]['name'];
								this.allImageLinks[lightboxedLinksTotal]['showFunction']= searchDefsToUse[ii]['showFunction'];
								this.allImageLinks[lightboxedLinksTotal]['link']	= links[i];
								lightboxedLinksTotal++;
								break checkLink;
							}
						}
					// for links that contain images that reveal larger image's location
					} else if (searchDefsToUse[ii]['findImageRegExp']) {
						if(this.containsThumb(links[i], searchDefsToUse[ii], false)) {
							if(searchDefsToUse[ii]['linkRegExp'].test(links[i].getAttribute('href'))) {
								if (!searchDefsToUse[ii]['excludeLinkRegExp'] || !searchDefsToUse[ii]['excludeLinkRegExp'].test(links[i].getAttribute('href'))) {
									addEvent(links[i], 'click', searchDefsToUse[ii]['showFunction']);
									/*for (ii = 0; ii < lightboxedLinksTotal; ii++) {
										if(greasedLightbox.allImageLinks[ii]['link'] == links[i]) break checkLink;
									}*/
									this.allImageLinks[lightboxedLinksTotal]	= new Array(3);
									this.allImageLinks[lightboxedLinksTotal]['name']= searchDefsToUse[ii]['name'];
									this.allImageLinks[lightboxedLinksTotal]['showFunction']= searchDefsToUse[ii]['showFunction'];
									this.allImageLinks[lightboxedLinksTotal]['link']= links[i];
									lightboxedLinksTotal++;
									break checkLink;
								}
							}
						}
					}
				}

			} // end if()
		} // end for()
		
		if (lightboxedLinksTotal == 0) return;
		
		addEvent(window, 'unload', this.unload);
		
		var objBody							= document.getElementsByTagName("body").item(0);
		
		var pngOverlay						= "url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAABGdBTUEAAK%2FINwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAAPSURBVHjaYmJgYDgDEGAAANsAz1TKIeAAAAAASUVORK5CYII%3D')";
											   
		var lightbulbOffIcon				= "url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAABGdBTUEAAK%2FINwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAJOSURBVDjLlZLdTxJQGMa96K4%2FoPUHdFfrpntyZrXsoq25tlbroi6qi7ZuYsuZ0UXRWiv72NS0gjIgDQ1LS0wkwU%2FUVEREUkEIBBFE%2BV48ve%2FZICza7OLZOTt739%2FznHPeEgAlhZpyB8%2BMLa58HHL63H2zy4muycVku8UZahl2TNJ688%2F6wsbd31yBLps3BNdqFCvrMYRjSURIvOdzzdAcmozWhTaLc%2B8WADXvHHb6RhYCEdEU2kiIJu%2FaBtwEywE3k2lQKjz8NB7Sjs7vygPMDu9ddogmUliNxsWaSGfwM5sViqcy%2BBHeFCl4r6YkzwzTnXlA9%2FSSh924md25qFDszMnmfGuga4pEd3QjiTxAN%2F49xY0c10MgjsuOuSssBdfh8IdBSUG1AibTDmbzAHrhZab6IzHQq6N3xo3%2BLyqY%2B1phMmig%2F9AISolm8yyMdo9IcKtt6HcC%2Bh653uoScTsJ0K65jw5yYrWOOISrol6Kht4pcUV%2Bg0efJwx5ADXtUA3a7aMLflHQoa0VzfTSoHMBUClqwL9EM4Lrb01JOt%2BzZQ7ob%2Fc%2FN1qDDGEHBugKxO6mOS%2BqWswZRb%2Ft9F%2BDxCLHAzQovsfdEyAYXn6d4cHBa7r7NXU%2FbrwbiCpNtsNFJzEnaqp4KjufblDU4XbtJVTJL%2BBqjQynyvZl6e8P%2FnOUC1UtvehWNr%2BBUqlGXX0T7j14gpMVZcFitUUB0ivnBvQ9PQgEgrBYxvBC8QqVxyXz2wboVfKzlSeOxsrLD2VLSyXZY0ck8feN1Ze3Dfgf%2FQJBCig%2B4GhFlwAAAABJRU5ErkJggg%3D%3D')";
		
		// CSS
		var head, styleSheet, cssStr, cssText;
		head = document.getElementsByTagName('head')[0];
		styleSheet						= document.createElement('style');
		styleSheet.setAttribute('id','greasedLightboxCSS');
		styleSheet.setAttribute('type','text/css');
		head.appendChild(styleSheet);
		
		cssStr							= ''+
		'#greasedLightboxOverlay { position: absolute; top: 0; left: 0; z-index: 10000000; width: 100%; background-image: ' + pngOverlay + '; background-repeat: repeat; cursor: pointer; }'+
		
		'#greasedLightboxMenu { position: fixed; top: 0; left: 0; width: 100%; z-index: 10000100; background: #000; font-family: "Terbuchet MS", Tahoma, Arial, Verdana, sans-serif; font-size: 14px; font-weight: bold; height: 35px; line-height: 35px; opacity: .5; }'+
		'#greasedLightboxMenu:hover { opacity: 1; }'+
		'a#greasedLightboxTitleLink { position: absolute; top: 0; left: 0; display: block; height: 35px; line-height: 35px; margin: 0 5px; padding: 0 5px 0 27px; background-image: ' + lightbulbOffIcon + '; background-repeat: no-repeat; background-position: 5px 55%; color: #aaa; background-color: #000; text-decoration: none; cursor: pointer; z-index: 10000450; }'+
		'a#greasedLightboxTitleLink:hover { color: #fff; background-color: #333; }'+
		'#greasedLightboxButtons { position: absolute; top: 0; left: 0; height: 35px; width: 100%; line-height: 35px; margin: 0; padding: 0; z-index: 10000400; }'+
		'#greasedLightboxButtons a { display: block; width: 33px; height: 33px; border: 1px solid #000; background: #000; cursor: pointer; float: right; text-align: center; color: #aaa; z-index: 10000450; }'+
		'#greasedLightboxButtons a:hover { border-color: orange; background-color: #333; color: #fff; }'+
		'#greasedLightboxLoading { position: absolute; z-index: 10000050; color: #fff; font-weight: bold; font-family: "Trebuchet MS", Tahoma, Arial, Verdana, sans-serif; text-align: center; line-height: 2em; }'+
		'p#greasedLightboxLoadingText { margin: 0; padding: 25px 0 5px 0; font-size: 45px; color: #fff; font-weight: bold; font-family: "Trebuchet MS", Tahoma, Arial, Verdana, sans-serif; line-height: 1em; text-align: center; }'+
		'p#greasedLightboxLoadingHelp { margin: 0; padding: 5px 0; font-weight: normal; font-size: 11px; color: #fff; font-family: "Trebuchet MS", Tahoma, Arial, Verdana, sans-serif; line-height: 1em; text-align: center; }'+
		'#greasedLightboxError { position: absolute; z-index: 10000050; text-align: center; background: #000; color: #aaa; padding: 10px; border: 1px solid #444; -moz-border-radius: 10px; font-family: verdana, sans-serif; font-size: 11px; }'+
		'p#greasedLightboxErrorMessage { color: #fff; font-size: 45px; font-weight: bold; margin: 10px 20px; font-family: "Trebuchet MS", Tahoma, Arial, Verdana, sans-serif; text-decoration: none; border: none; text-align: center; }'+
		'#greasedLightboxError a, #greasedLightbox a { color: #aaa; text-decoration: none; border-bottom: 1px solid #777; }'+
		'p#greasedLightboxErrorContext { margin: 0; padding: 5px 0; font-weight: normal; font-size: 11px; color: #fff; font-family: "Trebuchet MS", Tahoma, Arial, Verdana, sans-serif; line-height: 1em; text-align: center; }'+
		'#greasedLightbox { position: absolute; z-index: 10000050; text-align: center; background: #000; color: #aaa; padding: 10px; border: 1px solid #444; -moz-border-radius: 10px; font-family: verdana, sans-serif; font-size: 11px; }'+
		'img#greasedLightboxImage { border: none; cursor: pointer; }'+
		'img#greasedLightboxImage, img#greasedLightboxPreload, img#greasedLightboxPrefetch {  max-height: none; max-width: none; }'+
		'#greasedLightbox, #greasedLightboxMenu, #greasedLightboxOverlay, #greasedLightboxError, #greasedLightboxLoading, img#greasedLightboxPreload, img#greasedLightboxPrefetch { display: none; }'+
		'#greasedLightboxCaption { color: #aaa; padding: 10px 0; }';
		
		//cssText							= document.createTextNode(cssStr);
		//styleSheet.appendChild(cssText);
		styleSheet.styleSheet.cssText = cssStr;
		
		// overlay div
		var objOverlay						= document.createElement("div");
		addEvent(objOverlay, 'click', greasedLightbox.hide);
		objOverlay.setAttribute('id','greasedLightboxOverlay');
		objBody.insertBefore(objOverlay, objBody.firstChild);
		
		// menu div
		var objMenu							= document.createElement("div");
		objMenu.setAttribute('id', 'greasedLightboxMenu');
		objBody.insertBefore(objMenu, objBody.nextSibling);
		
		// title link
		var objMenuLink						= document.createElement("a");
		objMenuLink.setAttribute('id', 'greasedLightboxTitleLink');
		objMenuLink.setAttribute('href', 'http://shiftingpixel.com/lightbox/');
		objMenuLink.innerHTML					= 'Greased Lightbox';
		objMenu.appendChild(objMenuLink);
		
		// menu buttons div
		var objMenuButtons					= document.createElement("div");
		objMenuButtons.setAttribute('id', 'greasedLightboxButtons');
		objMenu.appendChild(objMenuButtons);
		
		// right button
		var objMenuButtonRight				= document.createElement("a");
		objMenuButtonRight.setAttribute('id', 'greasedLightboxButtonRight');
		objMenuButtonRight.setAttribute('title', greasedLanguage[greasedLanguage.language][0].next);
		objMenuButtonRight.innerHTML		= '\u2192';
		addEvent(objMenuButtonRight, 'click', function(e) { greasedLightbox.moveSlide(e, 1); });
		objMenuButtons.appendChild(objMenuButtonRight);
		
		// left button
		var objMenuButtonLeft				= document.createElement("a");
		objMenuButtonLeft.setAttribute('id', 'greasedLightboxButtonLeft');
		objMenuButtonLeft.setAttribute('title', greasedLanguage[greasedLanguage.language][0].previous);
		objMenuButtonLeft.innerHTML			= '\u2190';
		addEvent(objMenuButtonLeft, 'click', function(e) { greasedLightbox.moveSlide(e, -1); });
		objMenuButtons.appendChild(objMenuButtonLeft);
		
		// magnify button
		var objMenuButtonPlus				= document.createElement("a");
		objMenuButtonPlus.setAttribute('id', 'greasedLightboxButtonPlus');
		objMenuButtonPlus.setAttribute('title', greasedLanguage[greasedLanguage.language][0].magnify);
		objMenuButtonPlus.innerHTML		= '+';
		addEvent(objMenuButtonPlus, 'click', function(e) { greasedLightbox.resize(e, 13); });
		objMenuButtons.appendChild(objMenuButtonPlus);
		
		// shrink button
		var objMenuButtonMinus				= document.createElement("a");
		objMenuButtonMinus.setAttribute('id', 'greasedLightboxButtonMinus');
		objMenuButtonMinus.setAttribute('title', greasedLanguage[greasedLanguage.language][0].shrink);
		objMenuButtonMinus.innerHTML		= '-';
		addEvent(objMenuButtonMinus, 'click', function(e) { greasedLightbox.resize(e, -13); });
		objMenuButtons.appendChild(objMenuButtonMinus);
		
		// loader div
		var objLoading						= document.createElement("div");
		objLoading.setAttribute('id','greasedLightboxLoading');
		addEvent(objLoading, 'click', greasedLightbox.hide);
		objBody.insertBefore(objLoading, objBody.nextSibling);
		
		var loadingGif = document.createElement('img');
		loadingGif.src = "data:image/gif,GIF89a%80%00%80%00%A2%00%00%FF%FF%FF%DD%DD%DD%BB%BB%BB%99%99%99%00%00%FF%00%00%00%00%00%00%00%00%00!%FF%0BNETSCAPE2.0%03%01%00%00%00!%F9%04%05%05%00%04%00%2C%02%00%02%00%7C%00%7C%00%00%03%FFH%BA%DC%FE0%CA%06*%988%EB%CD%BB_%96%F5%8Ddibax%AEl%AB%A5%A2%2B%CF.%5C%D1x%3E%DA%97%EE%FF%12%1EpHT%08%8B%C8G%60%190%1DI%83%E8%20%F9a2K%CF%8FTJ%E5X%AD%A4lg%BB%EDj%BE%D7%9D%0DJ%8E%9A3%E8%B4G%BCis%DF%93%B8%9CC%CF%D8%EFx%12zMsk%1E%7FS%81%18%83%850%87%7F%8Apz%8D)%8Fv%91%92q%1D%7D%12%88%98%99%9A%1B%9C%10%88%89%9Fy%93%A2%86%1A%9E%A7%8B%8C%2F%AB%18%A5%AE%A0_%AA%8E%AC%90%B5%B6%60%19%A3%0D%AD%BC%AF%A1(%B2%9D%BB%C3%C4h%BF%C7%A4%C9%CA%A8%A9A%CE%0E%B4%D1%BD%7B%10%C0%0A%C2%D8%D2%C5%DB%D5%0C%D7%DF%CB%B7%13%B9%C8%97x%02%EE%02%2B%B0%D47%13%DEln%1E%EF%EF'%F2%2B%F6Zd%3A%E8%1Bhb%9A%3Fv%F7%DAp%18%C8%90%84%C1%13%D0%C6%94%CB%C0%B0%E2%08f2%14%02%2Ce%8A%FFb%C5%86U%B4%B5(%B3%91%A3%C0%8F%20%CD%CD%E2%08h!%CA%94*%AD%B1l%99%EF%25%C1%98%0Bf%D2%1Ca%F3fL%9D%F8X%F4%D4g%0EhG%17C%F7%0D3%EA%23%A9%3B%5EL%818u%054%C9P%AA%2C%DF%D8%C4%FA%8F%CAK%AE%08%15Y%AC%15%F6%13%D1%A5%3Bq%AA%5D%CB%B6%AD%DB%B7p%E3%CA%9DK%B7%AE%DD%BBx%F3B4%DA%F5%1B_a%7F'%16%0D%0C%89%B0%E0h%86%13%F3%FD%A9%B8qV%95%8E%23%F7%85*%D9Me%B5%97%BB9f%1BY%AF%E7%CF%A0C%8B%1EM%BA%B4%E9%D3%A8S%AB%C6A%92r%D0Se1%C5~s8P%ED%24%26a%DF%1E2%13%EC%E4%1CUu%F7%06%12%D5wn%E0%C1%5D%0F%9FQ%1Cq%F2%83%3A1%3FO%F8Xzt%EA%C7%DB6%AFs%5D%EE%F4%95%D5%25%BEv%D1Z%7Cv%F0%BB%EB%05%CC%B8%DERz%99%BF%D5kd%11%91%C3y%F9%F3G%D4%2F%B1%DF~%FF%08%BC%F9%E9%F7_I%EDaW%12t%01%3EP%DE3%B3%B9g%DB%80%9A-%A8%20%84%8CAha%7C%90Q%A8!%85~a%B8!%87%CE5%18%8C%88%E4%80%88%16%89%25%26%C8%A0%8A%19%A2%98%93%8B%11%B2%D8!%8C1J%08%A0%89%9F%BC%97b%81%F8%C9x%A2%8F%F0%F1%D8%A3%8D%CA%E8%B8%23%91%2B%02)%9C%92%232y%24%92%C6%A55%E4x~%E0H%9B%95%04%60%89%A1%22%5B%06%09%E5%8D4%9Aa%A4%97RNY%26%97X%D6x%E6%3ANv%91%A6%9ATr%D7%26%15of%19%26%99q%E6(%A4%7Fs%929%E3Q%EE%7D%89%1Eiu%AAVhj%87%A2%96%E8i%8B%9A%D6%A8%A3%7B%AE%C6'%A0%AE%24%00%00!%F9%04%05%05%00%04%00%2C%0A%00%02%00W%000%00%00%03%FFH%BA%DC%FE0%BE%40%83%BC8%EB%3D%2B%E5%60(J%9E7%9E(WVi%EBv%EB%2B%BF%EB7%DFgm%E1%3C%A8%F7%23%81P%90%FA%A1%00H%40k8D%19G%C9%24%8A%C9%CC%D5N%D1%E8%89%DA%1C%3DCYi%90%2B%F4%5EEa%B1%88%DC%F5%9DAi%F5%9A-%FAn%E2%CA%14%9B%E8%8E%C1%E3.%7B!v%19x%2F%82*o%1A%86%87%88%1A%84%12xy%8Dd%89~%8B%803%7B%7C%19%90%10%928%8E%18%9E%0F%8C%A1t%9D%8A%91%99%3C%A2%24%AA%11%A6%AD%A8%17%A4%0C%B2%B3%B4%11%B6%0A%A0%40%0A%AE0%25%18%B8%3D%9B%B5%B0%0D%BE%BF%C0%BA%10%97%B1%AC%10%03%D4%03%81%CE%C2%C4%D2%0F%D5%D5K%D8G%DB%0D%DD%E4z%952%E2%E3%E4%E5c%5C3%E9%0C%EB%F2%EDm%E8Y%18%F2%F3se%3CZ%19%F9%FA%98%09%04%18P%E0%2F%82%EB%0C2C%C8N!%10%86%DD%1C%1E%84HMb%0F%8A%15-%F2%C0%A8%F1%13%22%C3%8E%0F%09%82%0C%99o%E4%C4%86%26IZK%A9!%01%00!%F9%04%05%05%00%04%00%2C%1F%00%02%00W%000%00%00%03%FFH%BA%DC%FEKH%01%AB%BD8%EB6%E7%FE%60%A8u%9Dh%9E%22%E9%A1l%5B%A9%92%2B%CF%04L%D1%F8i%E7%7C%B8%F7%A2%81p%C0%FA%9D%02%C8%40k8D%19E%C9%24%8A%C9%D4%C1%8EQ%A9%89%DA4%3DAYm%90%2B%F4%5E%A1a%E4%89%DC%05%7D5i%F1%98%9C%3A%83%E3K%B6%CF%BE%89%2B%F3tn%7Cpx.lD%1Fo%17~3%87%88%23%83%8B%8C%8Dz%1B%8A%15%93%94%95%19%97%0F~%7F4%87%96%91%98%859%A2%9C%A4%9E%A6%A7%9B%17%9D%0D%99%3C%A8%AF%AA%B1%AC%B3%B4%2F%B6%0B%9F%40%0B%BA%10%B0%0A%B2%40%8E%B5*%92%B8%C6%AE%C2%24%18%C5%BF%04%C1%0F%25%CAa.%00%DA%00%18%D4(%D1!%DB%DB%DD%812%CB%20%E2%E9%17%CD%2C%E7%1A%E9%F0%E4U8%D8%22%F0%F7%19%F39Q%26%F7%F8%D2%D2%FC%FD%03%D8C%E0%40%828%0C%C6C%C8C%A1%3A%86%09%1D%8E%83HC%E2D%8A3%2Cj%C3X%D1%14%22%C7%88%0A%3F%E6%08)r%A4%C0%92%05%17%A2L%B9%D1D%02%00!%F9%04%05%05%00%04%00%2C%3C%00%02%00B%00B%00%00%03%FEH4%3C%FA0%CAI%AB%9D%AD%DD%CD%7B%CD%99'%8E%16%A8%91hj2i%3B%AE%8E%2Bo%F0l%7F%EB%ADG%B5%2B%FC%82%DD%A3%97%02%02%85%8B%5C%D1x%DC%11I%CC%A6%EE)%8AJo%D4%8E%F5j%CBr%B6A%A1%F7%02F%26M%D0%ADy%5C)%AF%95Z7%92%3D%91%CF%E1%1Bp%F8%8D%8E%5B%CDCx%16v%7C%20~Q%80%81%7Ddj%89%0At%0Az%8E%8F%82u%8D%93%90%92%93%94!%8C%7F%9B%8A1%83%97.%01%A6%01%3B%84(%A7%A7%3A%A4%AB%AC%AC7%AF%22%B1%B6%AEL)%B6%BB%A9%5C%1E%BB%BC%A0%1B%C0%C1%C2%15%C4%C5%C6%12%C8%B7%CA%14%CC%B1%CE%13%D0%B2%D2%11%D4%AD%D6%D7%D8%A8%DA%10%DC%DE%CB%D0%E1%D3%C8%E4%CF%C4%E7%C7%CD%EA%EB%A6%ED%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FA%00%FD%FE%FF%00%03%024%26%B0%A0%C1%7F%A0%0E*4%B8i%A1%C3%81%93%1EJ%04%D0p%A2%C3%84%16%0F%12%CC(%03PA%02%00!%F9%04%05%05%00%04%00%2CN%00%0A%000%00W%00%00%03%ECH%BA%BC%F3%A3%C9I%2B%85%D0%EA%7Dq%E6%E0%E6%7Da)%8D%A4%A9%A2%A9Z%B2%91%BB%B2%B2%0B%D7%E6%8D%87p%BCs%BA%9F((%B4%10%8B%1D%14r%A8%5CV%8ENF%2F%9A%1CQ'%D3k%03z%E5%AA%04%60%81%91%B6%0B%87%9F%CD%9Ay%5D%C5%A8%D7%EC%B6%CF%04%AF%8F%1F%B2%BA%9D%AA%DF%3B%FB~H%80p%7C%83fQ%86%87%7F%89%60%85%8C%8E%86Z%89Z%0A%83%94%0B%80%97%0C%81%9A%95g%9D%A0%A1%A2%A3%A4%A5%A6%A7%A8%A9%AA*%01%AD%AE%AF%B0%B1%B05%B2%B5%B6%AF.%B7%BA%B6%AC%BB%BE%B8%26%BF%C2%01%BD%C3%BB%B9%C6%B7%B4%C9%B2%AB%CE%CF%D0%D1%D2%D3%D4%D52%00%D8%00%A5%D9%DC%A2%DC%DF%DA%9D%E0%DF%E2%E3%E4%94%E6%E3%E8%E9%E0Z%EC%ED%EE%EF%DD%F1%F2%D8%F4%F5%EB%F5%E1W%FA%FB%FC%F8%F9%D8%95K%17%8A%A0%B7s%A3%E6QH%00%00!%F9%04%05%05%00%04%00%2CN%00%1F%000%00W%00%00%03%E9H%BA%DC%FEn%C8%01%AB%BDmN%CC%3B%D1%A0'F%608%8Eez%8A%A9%BAb%AD%FBV%B1%3C%93%B5v%D3%B9%BE%E3%3D%CA%2F%13%94%0C%81%BD%231%A8D%B6%9A%8F%1C%14R%9B%F2L%D6%AB0%CB%EDz%BF%E0%B0xL.%9B%CF%5C%81z%CDn%BB%DB%B3%B7%7C%CE%5E%D1%EF%F3%13~%0F%1F%F1%FF%02z%80%7Bv%83tq%86oh%8B%8C%8D%8E%8F%90%91%92%93%0A%01%96%01f%97%9Ac%9A%9D%98%60%9E%9D%A0%A1%A2%5D%A4%A1%A6%A7%9E%5C%AA%AB%AC%AD%9B%AF%B0%96%B2%B3%A9%B3%9FY%B8%B9%10%00%BE%00%2F%B8%15%BF%BF%C1%B0%BD%C4%C5%C6%A7%C8%C9%C07%CC%0F%CE%CA%D0%A5%D2%D3%CF%3B%B1%C3%D8b%D8%BE%DE%DDa%DF%D9_%DFc%E7%E3%E2%EA%D3%E1%EB%E6%EF%5E%E4%EE%CE%E8%F1%5D%E9%EC%F5%FA%FB%60%F9%FE%ED%E8%11%23%D3%CF%1E%B8)%09%00%00!%F9%04%05%05%00%04%00%2C%3C%00%3C%00B%00B%00%00%03%F9H%BA%DC%FEP%8DI%AB%BD6%EA%1D%B1%FF%15'r%60%F9%8D%E8c%AEY%EAJl%FC%BE%B1%3C%BB%B5y%CF%F9%B9%FF%C0%A0pH%2C%1A%8F%C8%A4r%C9l%3A%9F%D0%A8tJ%10X%05%D4%D7u%9B%1Dm%BF%D8%AE%06%FC%15G%C8%60%B3%03MV3%D8mw%15%5E%96%CF%E9W%FB%1D%1Fv%F3%F3v%7FVz%82F%01%87%017%7FD%88%88%8AxC%8D%8D%3Bt%91%92%87%40l%96%97%89%99u%11%00%A1%00%1C%9C%98A%5C%1A%A2%A2%A4%A5O%AA%AA%1B%A5%A6L%AF%AB%B1%ADM%B5%A1%AC%B8K%BA%A3%BC%97%B9%BA%23%B2%B4%C4%22%C6%BE%C8%C9%BDH%BF(%B2%9D%CF%CC%CD%9CJ%D0%D1%CAG%D9%DA%D7%D4%B5%2F%DBE%DD%DE%C2%DC%D5%E6%92%E8%E1%E2%E3B%E5)%EFA%F1%F2%DFD%F5%EA%8E%E4%E9.%E7%FC%EDvLb%F7J%8F%83%7Cv%10%CAQ%E8%86%A1%1A%87%0F%0B%1A%7C%00kb%83%04%00!%F9%04%05%05%00%04%00%2C%1F%00N%00W%000%00%00%03%FFH%BA%DC%FE0%CA7%EA%988%EB%CD%89%FD%5D(%8E%CDg%5Ed%AAJ%A7%B9%BE%B0%D7%BAq%1D%CE%AD%ADkx%BE%FF%90%DE%09Ht%08i%C5%E4%11%94%2C.-M%E5%13%15%05N5%80%2C%E0'%E8%0AFO%8CV%AB%F3z%C1%C7%C9x%5C3%9BIB%F5%3A%DBvwU8%C9%9C%1C%B3%9F%F1H%10%7Bt%13%01%86%01%18~w%2BL%11%83%5B%85%87%86%89%8AQ%8F%90%11%92%92%13%8A%8BE%8F%18%9A%87%94~I%97%A1%A2%88%9C%9D%9F%83%19%A9%AA%AB%A5%40%A0%AF%A9%1A%AC%3F%B5%A8%A2%B8%95%3B%BB%BC%9A%1B%B95%A7%1A%B0%C4%C50%C1%C2%9B%CA%B3%CC%CD%91%BD%D0%D1%2B%D3%D4%C3%1C%CB)%D9%DA%CF%DC%BF*%DF%12%C9%1D%DD%22%E5%E6%B7!%E9%1C%C7%1D%E7%E8%EFX%AE%22%F3%F4%D7%1D%F7%F8%ED%22%E3B%F4%0B%91O%9F%1BokR%144%E8%89%04%1B%85%FFF%BC%A9%E2l%14%C5(%0B%2F%FE%C8%A8Q%13%07%C7%8E5%3E%82%84!r%E4%8Bj%26%89%84K%A9%20%01%00!%F9%04%05%05%00%04%00%2C%0A%00N%00W%000%00%00%03%FFH%BA%DC%0E%10%B8I%AB%BD8%B7%C8%B5%FF%E0%C7%8DRh%9E!9%A2lK%A9%A4%2B%B7%B0%3A%DF%60m%E3%3C%A6%C7%BD%E0%E4%B7%12%1A%17%C4%CEq%99%8C%2C%8FM%C8%13%DA%9CR%89%A7%806%20%1Cx%07%99dv%AB%ED%7D%BF%3E%1D%8AL%C6%9D%CF%97Z%8B%BDu%BF%BDi%25%8B%5E%BF%DD%D1qN.%7Ce%17%02%87%02%18%7FxV%04%84%5C%86%88%87%8A%8BV%8F%90%15%92%92%17%8B%8CK%8F%18%9A%88%94%7FO%97%A1%A2%89%9C%9D%9F%84%19%A9%AA%AB%A5F%A0%AF%A9%1A%ACB%B5%A8%A2%B8%95A%BB%BC%9A%1E%B98%A7%1A%B0%C4%C53%C1%C2%9B%CA%B3%CC%CD%91%BD%D0%D1%83%AE%1F%C9%1F%CB%7B%D9%DA%B7%20%DDc%7C!%DB%DC%BF%DE%E5%E6%E1%E2%E9%26%C7%20%E7%E8%EF%20%D3%C8%ED%EE%D7%F6%EB%26%F3%FAo%D6%F4cW%CDD%3D~mP%FC%03%E8I%60!%85%F9%0C%02jDm%18E%2B%0B%2F%0A%C9%A8%B1%12%07%C7%8E8%3E%82%9C!r%A4%8C%82%26%8D%3C%E3%91%00%00!%F9%04%05%05%00%04%00%2C%02%00%3C%00B%00B%00%00%03%F5H%04%DC%FE%F0%A9I%AB%BD%98%C6%CD%5D%FE%E0%D5%8D%5Ch%82d*%9D%AC%A5%BE%40%2BO%B0%3A%DF%F5x%EF%F9%B6%FF%C0%A0pH%2C%1A%8F%C8%A4r%C9l%3A%9F%D0%A8tJ%3D%05%AE%81%AA%0C%CB%D5%9A%B8%E0%AC7%13%06%8F%2F%E5%F0%99%92.%AF%09m%F7%3A%AE%3E%D3%CD%F6%3B%F6%AD%DF%E7%FB%7C%80%81w%3B%02%86%02Fz%85%87%86Et%3F%8C%8CDmA%91%87%8Ex%40%96%97%98WC%9B%8D%20%03%A3%03R%A0%88%A2%A4%A3P%A7%A8%19%AA%AAO%A7!%B0%A4N%AD%B4%B5%A5M%B3%B9%B5%BC%A0'%BA%BBK%BD%BE%B0L%C6%C7%B1J%B8%C2%BA%C5%C1%2C%C3%CD%CA%CB%B6I%D6%D7%ABH%DA%DB%C4F%DE%A9%BFG%E2%E3%C8%E1%E6%1F%D4%E9%9B%3B%ECE%D27%F0D%F23%F4%F5%91%40%F8%F9%A1%3F%FCo%26%00%0CH%60%60%40%83o%10%AEQx%86aCt%0410K%00%00!%F9%04%05%05%00%04%00%2C%02%00%1F%000%00W%00%00%03%E7H%BA%0C%0E%2C%CAIk%7B%CE%EAM%B1%E7%E0%E6%8Da)%8D%A8%A9%A2%A9Z%B2%AD%CB%C1%B1%AC%D1%A4%7D%E3%98.%F2%0F%DF%0E%08%11v%88E%E3%04%A9%AC%00%9B%16%1C4%0A%9B%0E%7B%D6_%26%CB%EDz%BF%E0%B0xL.%9B%CF%A1%80z%CDn%BB%DB%B6%B7%7C%CEv%D1%EFs%15~%0F7%F1%FF%01z%80%7Bv%83tq%86oh%8B%8C%8D%8E%8F%90%91%92%93h%02%96%02f%97%9Ac%9A%9D%98%60%9E%9D_%A1%9E%5D%A4%A1Y%A7%A8V%AA%A5S%AD%A2%AF%B0%97%A9%B3%96%AC%B6%9F%B2%B3%5C%B62%03%C0%03%16%BC.%C1%C1%15%AD6%C6%C6%14%A7%3E%CB%C7%CD%B1%3A%D0%D1%D2%B7B%D5%C0b%DA%C2a%DD%DE%60%DD%DC%E3%DF%DA%E4%D5c%E5%E2%E7%E6%ED%EC%E9%EE%F1%F0%D0%E8%F5%F6%CB%F8%CC%F2%F7%F4%F9%FA%DB%D4%CD%D3wf%9F%86%04%00!%F9%04%09%05%00%04%00%2C%02%00%02%00%7C%00%7C%00%00%03%FFH%BA%DC%FE0%CAI%AB%BD8%EB%CD%BB%FF%60(%8Edi%9Eh%AA%AEl%EB%BEp%2C%CFt%0A%DC%40%AD%938%BE%FF%9E%5E%0FH%CC%08%7D%C5%24%E5%88T%3A%1D%CC%E6sJ%88%E6%A8X%2B%96%AA%DDN%BB%5E%A5%F5%1AN%82%CB%C41%DA%1C%5D%B3%99%EEt%3B%0E%3C%D3i%EA%BB%CE%AE%8F%E5%FB3%7C%80%12%01%85%01!%82%83%0E%86%86%20%89%8A%0B%8C%92%1Fs%90%10%92%98%1D%95%96%8B%98%99%1BG%9C%11%9E%9E%1CC%A2%A3%A4%9F%A8%26%AA%A5%AC%AD%AE%93%B0%24%B2%B3%B4%23%B6%8C%B8%B5%BA%85%BC%22%BE%BF%C0!%C2%C4%C1%B6%C7%B9%AE%CA%CB%A4%CD%BD%B7%D0%CE%87%D3%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF%E0%E1%C0%02%E4%E5%E6%E7%E8%E7%DC%E9%EC%ED%E6%DA%EE%F1%ED%D9%F2%F5%EA%D8%F6%F9%02%F4%FA%F5%F0%FD%EE%D6%01L'%AE%A0%C1%83%08%13*%5C%C8%B0%A1%C3%87h%06H%1C%00q%C1%C4%8B%10%2Fj%A4%D8pP%A3F%86%1E7*%0C%E9%11!%C9%92%07O%8A4%A8%F2%23%CB%96%13M%C2%94%98r%26%C7%970%13%CE%5C%98%93%E7I%87%24%2B%AE%ACH%00%23%D1%A3H%93*%5D%CA%B4%A9%D3%A7P%A3J%9DJ%B5%AA%D5%ABX%B3j%DD%CA%B5%AB%D7%AF%60%C3%16I%00%00%3B";
		loadingGif.style.border					= 'none';
		
		objLoading.appendChild(loadingGif);
		
		// loading text
		var objLoadingText					= document.createElement("p");
		objLoadingText.setAttribute('id','greasedLightboxLoadingText');
		objLoadingText.innerHTML			= greasedLanguage[greasedLanguage.language][0].loading;
		objLoading.appendChild(objLoadingText);
		
		// helper message
		var objLoadingHelp					= document.createElement("p");
		objLoadingHelp.setAttribute('id','greasedLightboxLoadingHelp');
		objLoadingHelp.innerHTML			= greasedLanguage[greasedLanguage.language][0].loadingSub;
		objLoading.appendChild(objLoadingHelp);
		
		// error div
		var objErrorBox						= document.createElement("div");
		objErrorBox.setAttribute('id','greasedLightboxError');
		objBody.insertBefore(objErrorBox, objBody.nextSibling);
		
		// error message
		var objError						= document.createElement("p");
		objError.setAttribute('id','greasedLightboxErrorMessage');
		//objError.innerHTML					= greasedLanguage[greasedLanguage.language][0].error + '<p id="greasedLightboxErrorContext"></p>';
		objErrorBox.appendChild(objError);

		// lightbox div
		var objLightbox						= document.createElement("div");
		objLightbox.setAttribute('id','greasedLightbox');
		objBody.insertBefore(objLightbox, objOverlay.nextSibling);
		
		// empty image
		var objImage						= document.createElement("img");
		addEvent(objImage, 'click', greasedLightbox.hide);
		objImage.setAttribute('id','greasedLightboxImage');
		objLightbox.appendChild(objImage);
		
		// empty preloader
		var objPreload						= document.createElement("img");
		objPreload.setAttribute('id','greasedLightboxPreload');
		objBody.insertBefore(objPreload, objBody.firstChild);
		
		// empty prefetcher
		var objPrefetch						= document.createElement("img");
		objPrefetch.setAttribute('id','greasedLightboxPrefetch');
		addEvent(objPrefetch, 'error', function() { return false; });
		objBody.insertBefore(objPrefetch, objBody.firstChild);
		
		// empty caption
		var objCaption						= document.createElement("div");
		objCaption.setAttribute('id','greasedLightboxCaption');
		objLightbox.appendChild(objCaption);
		
		//addEvent(document, 'keypress', greasedLightbox.handleKey);
		
/*
		// check for updates (once per day)
		try {
			var lastChecked					= GM_getValue('lastChecked', null);
			var latestVersion				= GM_getValue('latestVersion', null);
			
			var now							= new Date();
			now								= now.getTime();
			
			var oneDay						= 1000 * 60 * 60 * 24; // miliseconds * seconds * minutes * hours
			
			if (!lastChecked || (now - lastChecked) > oneDay) {
				GM_setValue('lastChecked', now.toString());
				GM_xmlhttpRequest({
					method: 'GET',
					url: 'http://shiftingpixel.com/downloads/greasedlightbox_latestversion.txt',
					headers: {
						'User-agent': 'Mozilla/4.0 (compatible) Greased Lightbox/'+greasedLightbox.version,
						'Accept': 'text/plain'
					},
					onload: function(responseDetails) {
						var latestVersion	= responseDetails.responseText;
						GM_setValue('latestVersion', latestVersion);
						
						if (latestVersion > greasedLightbox.version) greasedLightbox.showUpdate(latestVersion);
					}
				});
				
			} else if (latestVersion > greasedLightbox.version) {
				greasedLightbox.showUpdate(latestVersion);
			} // end if
		} catch (e) { }
*/		
	}, // end init()
	
	// unload
	// runs onunload to clear up possible memory leaks
	unload : function () {
		var objOverlay			= document.getElementById('greasedLightboxOverlay');
		removeEvent(objOverlay, 'click', greasedLightbox.hide);
		
		var objMenuButtonRight	= document.getElementById('greasedLightboxButtonRight');
		removeEvent(objMenuButtonRight, 'click', function(e) { greasedLightbox.moveSlide(e, 1); });
		
		var objMenuButtonLeft	= document.getElementById('greasedLightboxButtonLeft');
		removeEvent(objMenuButtonLeft, 'click', function(e) { greasedLightbox.moveSlide(e, -1); });
		
		var objMenuButtonPlus	= document.getElementById('greasedLightboxButtonPlus');
		removeEvent(objMenuButtonPlus, 'click', function(e) { greasedLightbox.resize(e, 13); });
		
		var objMenuButtonMinus	= document.getElementById('greasedLightboxButtonMinus');
		removeEvent(objMenuButtonMinus, 'click', function(e) { greasedLightbox.resize(e, -13); });
		
		var objLoading			= document.getElementById('greasedLightboxLoading');
		removeEvent(objLoading, 'click', greasedLightbox.hide);
		
		var objError			= document.getElementById('greasedLightboxErrorMessage');
		removeEvent(objError, 'click', greasedLightbox.hide);
		
		var objImage			= document.getElementById('greasedLightboxImage');
		removeEvent(objImage, 'click', greasedLightbox.hide);
		
		var objPrefetch			= document.getElementById('greasedLightboxPrefetch');
		removeEvent(objPrefetch, 'error', function() { return false; });
		
		removeEvent(document, 'keypress', greasedLightbox.handleKey);
		
	} // end unload()
} // end greasedLightbox

var greasedLanguage = {
	// Some of the translations here are very rough. If you can do better, please send me any changes (joe@shiftingpixel.com). I will be very happy.
	
	// english
	en : [
		{
			loading			: 'Loading image',
			loadingSub		: 'Click anywhere to cancel',
			context			: 'View image in its original context',
			error			: 'Image unavailable',
			next			: 'Next image (right arrow key)',
			previous		: 'Previous image (left arrow key)',
			magnify			: 'Magnify image (+ key)',
			shrink			: 'Shrink image (- key)',
			update			: 'Update available'
		}
	], // end english
	
	// español
	es : [
		{
			loading			: 'Cargando imagen',
			loadingSub		: 'Haz clic en cualquier sitio para cancelar',
			context			: 'Ver imagen en su contexto original',
			error			: 'La imagen no está disponible',
			next			: 'Siguiente imagen (tecla derecha)',
			previous		: 'Anterior imagen (tecla izquierda)',
			magnify			: 'Aumentar imagen (tecla +)',
			shrink			: 'Reducir imagen (tecla -)',
			update			: 'Actualización disponible'
		}
	], // end español
	
	// português
	pt : [
		{
		  	loading			: 'Imagem do carregamento',
			loadingSub		: 'Clique em qualquer lugar a cancelar',
			context			: 'Imagem da vista em seu contexto original',
			error			: 'Imagem unavailable',
			next			: 'Next image (right arrow key)',
			previous		: 'Previous image (left arrow key)',
			magnify			: 'Magnify image (+ key)',
			shrink			: 'Shrink image (- key)',
			update			: 'Update available'
		}
	], // end português
	
	// deutsch
	de : [
		{
		  	loading			: 'Bild wird geladen',
			loadingSub		: 'Zum Abbrechen irgendwo klicken',
			context			: 'Bild im ursprünglichen Kontext anzeigen',
			error			: 'Bild nicht verfügbar',
			next			: 'Nächstes Bild (Pfeil rechts)',
			previous		: 'Vorheriges Bild (Pfeil links)',
			magnify			: 'Bild vergrößern (+ Taste)',
			shrink			: 'Bild verkleinern (- Taste)',
			update			: 'Aktualisierung verfügbar'
		}
	], // end deutsch
	
	// français
	fr : [
		{
			loading			: 'Chargement de l\'image',
			loadingSub		: 'Cliquez n\'importe où pour annuler',
			context			: 'Voir cette image dans son contexte original',
			error			: 'Image indisponible',
			next			: 'Image suivante (Touche flèche droite) ',
			previous		: 'Image précédente (Touche fléche gauche)',
			magnify			: 'Agrandir l\'image (Touche +)',
			shrink			: 'Reduire l\'image (Touche -)',
			update			: 'Mise à jour disponible '
		}
	], // end français
	
	// het Nederlands
	nl : [
		{
			loading			: 'Laden',
			loadingSub		: 'Klik ergens om terug te keren',
			context			: 'Bekijk het plaatje in zijn originele context',
			error			: 'Plaatje niet beschikbaar',
			next			: 'Volgend plaatje (rechter pijltjestoets)',
			previous		: 'Vorig plaatje (linker pijltjestoets)',
			magnify			: 'Vergoot plaatje (+ toets)',
			shrink			: 'Verklein plaatje (- toets)',
			update			: 'Update beschikbaar'
		}
	], // end het Nederlands
	
	// italiano
	it : [
		{
		  	loading			: 'Scarico immagine',
			loadingSub		: 'Fai clic sullo sfondo per annullare',
			context			: 'Mostra nel suo contesto originale',
			error			: 'Immagine non disponibile',
			next			: 'Successiva (tasto freccia a destra)',
			previous		: 'Precedente (tasto freccia a sinistra)',
			magnify			: 'Ingrandisci (tasto +)',
			shrink			: 'Riduci zoom (tasto -)',
			update			: 'Aggiornamento disponibile'
		}
	], // end italiano
	
	// ????????
	el : [
		{
		  	loading			: '?????a f??t?s??',
			loadingSub		: '?t?p?ste ?p??d?p?te ??a ?a a????sete',
			context			: '?????a ?p???? st? a????? p?a?s?? t??',
			error			: '?????a µ? d?a??s?µ?',
			next			: 'Next image (right arrow key)',
			previous		: 'Previous image (left arrow key)',
			magnify			: 'Magnify image (+ key)',
			shrink			: 'Shrink image (- key)',
			update			: 'Update available'
		}
	], // end ????????
	
	// russki
	ru : [
		{
		  	loading			: '??????????? ????????',
			loadingSub		: 'Click ???-????, ????? ????? ????????',
			context			: '??????????? ??????? ? ????? ????????????? ??????',
			error			: '??????????? ?????????????',
			next			: 'Next image (right arrow key)',
			previous		: 'Previous image (left arrow key)',
			magnify			: 'Magnify image (+ key)',
			shrink			: 'Shrink image (- key)',
			update			: 'Update available'
		}
	], // end russki
	
	// hungarian
	hu : [
		{
			loading			: 'K\u00E9p bet\u00F6lt\u00E9se',
			loadingSub		: 'Kattints a visszal\u00E9p\u00E9shez',
			context			: 'Megtekint\u00E9s az eredeti k\u00F6rnyezet\u00E9ben',
			error			: 'K\u00E9p nem el\u00E9rhet\u0151',
			next			: 'K\u00F6vetkez\u0150 k\u00E9p (jobbra gomb)',
			previous		: 'El\u0150z\u0150 k\u00E9p (balra gomb)',
			magnify			: 'Nagy\u00EDtás (+ gomb)',
			shrink			: 'Kicsiny\u00EDt\u00E9s (- gomb)',
			update			: 'El\u00E9rhet\u0150 az \u00FAjabb verzi\u00F3'
		}
	], // end hungarian
	
	// finnish
	fi : [
	  {
		loading				: 'Ladataan kuvaa',
		loadingSub			: 'Napsauta kerran keskeyttääksesi',
		context				: 'Näytä kuva alkuperäisessä kontekstissa',
		error				: 'Kuvaa ei saatavissa',
		next     			: 'Seuraava kuva (oikea nuolinäppäin)',
		previous 			: 'Edellinen kuva (vasen nuolinäppäin)',
		magnify  			: 'Suurenna kuvaa (+ näppäin)',
		shrink   			: 'Pienennä kuvaa (- näppäin)',
		update   			: 'Päivitys saatavilla'
	  }
	], // end finnish
	
	// japanese
	ja : [
		{
		  	loading			: '\u8AAD\u307F\u8FBC\u307F\u4E2D',
			loadingSub		: '\u30AF\u30EA\u30C3\u30AF\u3067\u30AD\u30E3\u30F3\u30BB\u30EB\u3057\u307E\u3059',
			context			: '\u5143\u306E\u753B\u50CF\u3092\u8868\u793A',
			error			: '\u753B\u50CF\u304C\u5B58\u5728\u3057\u307E\u305B\u3093',
			next			: '\u6B21\u306E\u753B\u50CF',
			previous		: '\u524D\u306E\u753B\u50CF',
			magnify			: '\u753B\u50CF\u3092\u62E1\u5927 (+)',
			shrink			: '\u753B\u50CF\u3092\u7E2E\u5C0F (-)',
			update			: '\u65B0\u3057\u3044\u66F4\u65B0\u304C\u3042\u308A\u307E\u3059'
		}
	], // end japanese
	
	// chinese (simplified)
	zh : [
		{
		  	loading			: '\u8BFB\u53D6\u56FE\u7247',
			loadingSub		: '\u6309\u4EFB\u610F\u952E\u6765\u53D6\u6D88',
			context			: '\u4EE5\u539F\u6587\u672C\u67E5\u770B\u56FE\u7247',
			error			: '\u56FE\u7247\u4E0D\u53EF\u8BFB',
			next			: '\u4E0B\u4E00\u4E2A\u56FE\u7247 (\u53F3\u952E)',
			previous		: '\u524D\u4E00\u4E2A\u56FE\u7247 (\u56FE\u7247)',
			magnify			: '\u653E\u5927\u56FE\u7247 (+\u952E)',
			shrink			: '\u7F29\u5C0F\u56FE\u7247 (-\u952E)',
			update			: '\u53EF\u63D0\u4F9B\u66F4\u65B0'
		}
	], // end chinese (simplified)
	
    // chinese (traditional)
    tw : [
        {
            loading			: '\u8F09\u5165\u5716\u7247\u4E2D',
            loadingSub		: '\u6309\u4EFB\u610F\u9375\u53D6\u6D88',
            context			: '\u4EE5\u539F\u59CB\u9023\u7D50\u770B\u5716',
            error			: '\u7121\u6CD5\u8F09\u5165\u5716\u7247',
            next			: '\u4E0B\u4E00\u5F35\u5716 (\u53F3\u9375)',
            previous		: '\u4E0A\u4E00\u5F35\u5716 (\u5DE6\u9375)',
            magnify			: '\u653E\u5927\u5716\u7247 (+\u9375)',
            shrink			: '\u7E2E\u5C0F\u5716\u7247 (-\u9375)',
            update			: '\u6709\u66F4\u65B0\u7248\u672C'
        }
    ], // end chinese (traditional)
    
    // polish
	pl : [
		{
			loading			: '\u0141aduj\u0119 obraz',
			loadingSub		: 'Kliknij aby przerwa\u010B',
			context			: 'Zobacz obraz w oryginalnym kontek\u015Bcie',
			error			: 'Obraz niedost\u0119pny',
			next			: 'Nast\u0119pny obraz (klawisz \u2192)',
			previous		: 'Poprzedni obraz (klawisz \u2190)',
			magnify			: 'Powi\u0119ksz obraz (klawisz +)',
			shrink			: 'Zmniejsz obraz (klawisz -)',
			update			: 'Dost\u0119pna nowa wersja'
		}
	], // end polish 
	
	// czech
	cs : [
		{
			loading			: 'Nahrávám obrázek',
			loadingSub		: 'Klikněte kamkoliv pro zrušení',
			context			: 'Prohlížet obrázek v orignálním kontextu',
			error			: 'Obrázek není dostupný',
			next			: 'Další obrázek (šipka doprava)',
			previous		: 'Předchozí obrázek (šipka doleva)',
			magnify			: 'Přiblížit obrázek (klávesa +)',
			shrink			: 'Oddálit obrázek (klávesa -)',
			update			: 'Je dostupná aktualizace'
		}
	], // end czech
	 
	/* language template
	// 
	 : [
		{
		  	loading			: '',
			loadingSub		: '',
			context			: '',
			error			: '',
			next			: '',
			previous		: '',
			magnify			: '',
			shrink			: '',
			update			: ''
		}
	], // end 
	*/
	
	// lauguage
	// the correct language for localization is set in init()
	language : null,
	
	// init()
	// sets this.language to the correct value based on navigator.language
	init : function() {
		this.language		= this[navigator.userLanguage.substring(0,2)] ? navigator.userLanguage.substring(0,2) : 'en';
	} // end init()
}; // end greasedLanguage

if (document.body) greasedLightbox.init();

})();
