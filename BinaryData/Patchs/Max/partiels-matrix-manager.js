var track_ids = [];
var track_names = new Map();

function output_names()
{
    outlet(0, "clear");
	for (let track_name of track_names.values())
	{
		outlet(0, "append", track_name);
	}
}

function anything()
{
	var a = arrayfromargs(messagename, arguments);
	if(a.length <= 1)
	{
		return;
	}
	if(a[0] == "/tracks")
	{
		var new_track_ids = [];
		for (let index = 1; index < a.length; index += 2) 
		{
			if(a[index+1] == "vector")
			{
				new_track_ids.push(a[index]);
			}
		}
		for (let track_id of track_ids)
		{
			if(!new_track_ids.includes(track_id))
			{
				this.patcher.remove(this.patcher.getnamed("part" + track_id));
				track_names.delete(track_id);
			}
		}
		for (let index = 0; index < new_track_ids.length; ++index)
		{
			var track_id = new_track_ids.at(index);
			var objvarname = "part"  + track_id;
            var xpos = 20;
			var ypos = 20 + index * 40;
            var obj = this.patcher.getnamed(objvarname);
            if(!obj)
            {
                obj = this.patcher.newdefault(xpos, ypos, "partiels-plot-matrix-ebd", track_id);
                obj.varname = objvarname;
				track_names.set(track_id, track_id);
            }
            obj.rect = [xpos, ypos, 240, 80];
		}
		track_ids = new_track_ids;
		output_names();
	}
    else if(a[0] == "show")
	{
        messnamed(track_ids.at(a[1]) + "-receive", "thispatcher", "front")
	}
	else
	{
		var track_id = a[0].substring(1);
		if(track_ids.includes(track_id))
		{
			a.shift();
			if(a[0] == "name")
			{
				track_names.set(track_id, a[1]);
				messnamed(track_id + "-receive",  "thispatcher", "title", "Partiels - " + a[1]);
				output_names();
			}
			else
			{
                messnamed(track_id + "-receive", a)
			}
		}
	}
}