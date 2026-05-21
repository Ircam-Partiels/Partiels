# Improve the visualization of a group
Keywords: Visualization, Graphics, Colors, Readability, Layout, Figure, Display

1. Export the group as an image with `save_track_results`, then read and analyze the image. The hidden tracks of the group don't appear in the image.
2. If necessary, modify the graphical properties (`set_track_graphics`), the layout (`move_tracks` or the `layout` property of `set_group_properties`), or the zoom of the tracks (the `verticalZoom` and `valueRange` properties of `set_track_properties`) with the following requirements:  
    * The spectrogram (vector) tracks must be behind the other tracks (at the bottom of the group, move them with `move_tracks` and `"placement": "bottom"` if necessary).  
    * The visible tracks must use distinct colors (`colorForeground`).  
    * The tracks must use the same colors for the foreground and the text (`colorForeground` and `colorText`).  
    * The visible texts of the label tracks must have different positions (`labelLayoutPosition`).  
    * The line widths (`lineWidth`) and the font sizes (`fontSize`) of the tracks must be big enough to be readable.  
    * The visualization should be clear and highlight the analyses (e.g. `colorMap` and value range of the spectrograms, vertical zoom).
3. Go back to step 1 until the visualization is satisfying, with at most 3 iterations, then describe the changes to the user.
