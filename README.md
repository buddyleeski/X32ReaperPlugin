# X32ReaperPlugin
A growing feature set of two way interactions between Reaper and the X32 family of Behringer digital mixers, packages as a Reaper plugin (for that streamlined workflow that you always dreamed of)

## Current Features
1. Task menu option to reassign all track hardware inputs in a 1 to 1, 2 to 2 fashion with the behringer card outputs. 
   This is most useful when starting a new recording project. This doesn't actually do anything on the X32
2. Task menu option to sync reaper and x32 labels and set up hardware outputs
   This matches tracks to reaper channels based on a routing lookup. IE. (Track 1 = Input 1 = X32 Card Out 1 = X32 Channel 4)
   Next it will read the color values from the X32 and assign them to the Reaper Tracks. 
   Then the Reaper track labels get assign to the appropriate X32 Channel.
   Finally (Since I use the X32 Compact and only have 16 physical inputs), The reaper labels and colors get copied again to channels 17-32 (depending on how many tracks there are), and the track hardware outputs get assigned in order... Track 1 -> X32 Card Out 17....
   
## Next Steps
Add a feature to sync just channels 17-32. This is in the case that I already have a reaper project with tracks defined and want to start mixing on the X32 itself.
