{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 9,
			"minor" : 0,
			"revision" : 2,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"classnamespace" : "box",
		"rect" : [ 754.0, 114.0, 575.0, 510.0 ],
		"gridsize" : [ 15.0, 15.0 ],
		"boxes" : [ 			{
				"box" : 				{
					"id" : "obj-3",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 7.0, 481.0, 292.0, 20.0 ],
					"text" : "Partiels by Pierre Guillot at IRCAM IMR Department."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-12",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 116.0, 203.0, 236.0, 22.0 ],
					"text" : "id /9c0e3843c2ba4b4eb0e3c3812215b024"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-8",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 121.0, 177.0, 204.0, 20.0 ],
					"text" : "Change the id of the track to follow"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-5",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 325.0, 271.0, 234.0, 20.0 ],
					"text" : "1st argument: The id of the track to follow"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-13",
					"maxclass" : "multislider",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 7.0, 307.0, 552.0, 157.0 ],
					"setminmax" : [ -116.751991271972656, 0.0 ],
					"setstyle" : 1,
					"size" : 250
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-11",
					"linecount" : 4,
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 7.0, 78.0, 500.0, 60.0 ],
					"text" : "The partiels-plot-matrix abstraction stores and plots the OSC data received from a matrix track of the Partiels application into a multislider. Multislider size and value scales are automatically updated according to track zooms in the Partiels application. To run this patch, you should open the Partiels document Voice.ptldoc and enable the OSC."
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-9",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 7.0, 56.0, 320.0, 20.0 ],
					"text" : "Plots the matrix data received from the Partiels application"
				}

			}
, 			{
				"box" : 				{
					"fontsize" : 36.0,
					"id" : "obj-6",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 7.0, 7.0, 312.0, 47.0 ],
					"text" : "partiels-plot-matrix"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-4",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 7.0, 270.0, 316.0, 22.0 ],
					"text" : "partiels-plot-matrix /03cb5fd936574caebd686efde3a70f32"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-1",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 7.0, 158.0, 97.0, 22.0 ],
					"text" : "udpreceive 9001"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 0 ],
					"source" : [ "obj-1", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-4", 0 ],
					"midpoints" : [ 125.5, 255.0, 16.5, 255.0 ],
					"source" : [ "obj-12", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-13", 0 ],
					"source" : [ "obj-4", 0 ]
				}

			}
 ],
		"originid" : "pat-148",
		"dependency_cache" : [ 			{
				"name" : "partiels-plot-matrix.maxpat",
				"bootpath" : "~/Git/Partiels/BinaryData/Patchs",
				"patcherrelativepath" : ".",
				"type" : "JSON",
				"implicit" : 1
			}
 ],
		"autosave" : 0
	}

}