To run, click twice over MirIcoGen.jar (you need Java 6)


The overlay names have to be:
	offline.ico
	online.ico
	away.ico
	dnd.ico
	na.ico
	occupied.ico
	freechat.ico
	invisible.ico
	onthephone.ico
	outtolunch.ico


And the protocol icons have to be
	<protocol name>.ico


If you want, for some status, to not use the overlay, create an icon
	<protocol name>-<overlay name>.ico
inside the protos folder


You can also set transformations for the proto, overlay and/or out ico. To do that, create a file named
	<overlay name>.transforms
in the overlays folder. Inside it, each line has the format
	<target> <transform> <param>
where
	<target> is proto, overlay or out
	<transform> is grayscale, set_avg_lumi, bright or transp
	<param> depends on the transform (usually an integer)
There can be any number of those lines in the file. A # starts a comment.
For example, if you want to set the average luminescence of the protocol icon to 200, add some transparency to the overlay 
and make the out icon darker for the invisible icon, you can create an invisible.transforms containing:
	proto set_avg_lumi 200
	overlay transp +100
	out bright -100
	

To create packs for plugins, create for each plugin a folder and inside that folder adds all icons and a file named <plugin>.icons 
This file has lines in the format
	<ico name>=<id>
For example:
	url=138
To obtain the correct ids, you have to get the plugin source files and read the .rc file. 
If you create a .icons file for a plugin, please send it to me so I can add to the zip and other people can use it. 



Case matters!



Based on 
 - Miranda Icon Builder by Art Fedorov
 - ant
 - image4j
 - swt
 - velocity
 - jfg
 - commons-collections
 - commons-lang
