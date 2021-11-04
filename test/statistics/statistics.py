#%%
from mpl_toolkits.axisartist.parasite_axes import HostAxes, ParasiteAxes
import matplotlib.pyplot as plt
import numpy as np
import time, datetime
import os
import re

# get file name
file_names = os.listdir(os.getcwd())
#print(file_names)

spatial_layers_txt = ""
bitrate_txt = ""
delta_txt = ""
loss_txt = ""
audio_score_txt = ""
for file_name in file_names:
    #print(file_name)
    if re.search("spatial_layers.txt", file_name):
        spatial_layers_txt = file_name
    elif re.search("bitrate.txt", file_name):
        bitrate_txt = file_name
    elif re.search("delta.txt", file_name):
        delta_txt = file_name
    elif re.search("loss.txt", file_name):
        loss_txt = file_name
    elif re.search("audio_score.txt", file_name):
        audio_score_txt = file_name

print("spatial_layers_txt is empty!" if len(spatial_layers_txt) == 0 else spatial_layers_txt)
print("bitrate_txt is empty!" if len(bitrate_txt) == 0 else bitrate_txt)
print("delta_txt is empty!" if len(delta_txt) == 0 else delta_txt)
print("loss_txt is empty!" if len(loss_txt) == 0 else loss_txt)
print("audio_score_txt is empty!" if len(audio_score_txt) == 0 else audio_score_txt)

#spatial_layers###################################################
file = open(spatial_layers_txt, 'r')

#to 2 dimesion arrary
line = file.readline()
layer_array = line.strip().split(',')

layers = []
times_layers = []

for item in layer_array:
    #print(item)
    if item != '':
        layers.append(int(item.split('-')[0]))
        times_layers.append(int(item.split('-')[1]))
print("layers", layers)
#print(times_layers)
file.close()

#bitrate###################################################
file = open(bitrate_txt, 'r')
line = file.readline()
bitrate_array = line.strip().split(',')

bitrates = []
times_bitrate = []

for item in bitrate_array:
    if item != '':
        bitrates.append(int(item.split('-')[0]))
        times_bitrate.append(int(item.split('-')[1]))
print("bitrates", bitrates)
#print(times_bitrate)
file.close()

#delta###################################################
file = open(delta_txt, 'r')
line = file.readline()
delta_array = line.strip().split(',')

deltas = []
delta_times = []

for item in delta_array:
    if item != '':
        deltas.append(int(item.split('-')[0]))
        delta_times.append(int(item.split('-')[1]))

#print(deltas)
#print(delta_times)
file.close()

#loss###################################################
file = open(loss_txt, 'r')

#to 2 dimesion arrary
line = file.readline()
loss_array = line.strip().split(',')

loss = []
times_loss = []

for item in loss_array:
    print(item)
    if item != '':
        loss.append(float(item.split('-')[0]))
        times_loss.append(int(item.split('-')[1]))
print("loss:", loss)
#print(times_loss)
file.close()

#audio score###################################################
file = open(audio_score_txt, 'r')

#to 2 dimesion arrary
line = file.readline()
audio_score_array = line.strip().split(',')

audio_score = []
times_audio_score = []

for item in audio_score_array:
    print(item)
    if item != '':
        audio_score.append(float(item.split('-')[0]))
        times_audio_score.append(int(item.split('-')[1]))
print("audio_score:", audio_score)
#print(times_audio_score)
file.close()

#axes####################################################
title = ""

#data = [int(re.findall(r"\d+\.?\d*",x[6])[0]) for x in linesList]
#print(data)

#create HostAxes(bitrate) and ParasiteAxes(layers, delta, loss)##########
#figure define, what is 1?
fig = plt.figure(1) 
#use [left, bottom, weight, height]to define axes，0 <= l,b,w,h <= 1
ax_bitrate = HostAxes(fig, [0, 0, 2, 0.9])
#ax_bitrate.set_yticks([0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600])

#parasite addtional axes, share x
ax_layers = ParasiteAxes(ax_bitrate, sharex=ax_bitrate)
ax_delta = ParasiteAxes(ax_bitrate, sharex=ax_bitrate)
ax_loss = ParasiteAxes(ax_bitrate, sharex=ax_bitrate)
ax_audio_score = ParasiteAxes(ax_bitrate, sharex=ax_bitrate)
ax_audio_score.set_yticks([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

#append axes
ax_bitrate.parasites.append(ax_layers)
ax_bitrate.parasites.append(ax_delta)
ax_bitrate.parasites.append(ax_loss)
ax_bitrate.parasites.append(ax_audio_score)

#invisible right axis of ax_bitrate
ax_bitrate.axis['right'].set_visible(False)
ax_bitrate.axis['top'].set_visible(False)
ax_layers.axis['right'].set_visible(True)
ax_layers.axis['right'].major_ticklabels.set_visible(True)
ax_layers.axis['right'].label.set_visible(True)

#set label for axis
ax_bitrate.set_ylabel('bitrate(kbps)')
ax_bitrate.set_xlabel('time(ms)')
ax_layers.set_ylabel('layer(0,1,2)')
ax_delta.set_ylabel('delta(ms)')
ax_loss.set_ylabel('loss(0~100%)')
ax_audio_score.set_ylabel('audio score(0~10)')

#new axsi line 
load_axisline = ax_delta.get_grid_helper().new_fixed_axis
loss_axisline = ax_loss.get_grid_helper().new_fixed_axis
audio_score_axisline = ax_audio_score.get_grid_helper().new_fixed_axis

#axsi line padding on the right
ax_delta.axis['right2'] = load_axisline(loc='right', axes=ax_delta, offset=(40,0))
ax_loss.axis['right3'] = loss_axisline(loc='right', axes=ax_loss, offset=(80,0))
ax_audio_score.axis['right4'] = audio_score_axisline(loc='right', axes=ax_audio_score, offset=(120,0))

#add host axes to fig
fig.add_axes(ax_bitrate)

#plot fig####################################
#curve_layers = ax_layers.scatter(times_layers, layers, label="layer", color='red', s=50)
ax_layers.set_ylim(0, 2)

curve_bitrate = ax_bitrate.plot(times_bitrate, bitrates, 'o-', label="bitrate", color='green')

#curve_delta = ax_delta.scatter(delta_times, deltas, label="delta", color='blue', s=1)

curve_loss = ax_loss.plot(times_loss, loss, '.-', label="loss", color='orange')
ax_loss.set_ylim(0, 100)

curve_audio_score = ax_audio_score.plot(times_audio_score, audio_score, '.-', label="audio score", color='purple')

ax_bitrate.legend()

#axes name and color############################
ax_bitrate.axis['left'].label.set_color('green')
ax_layers.axis['right'].label.set_color('red')
ax_delta.axis['right2'].label.set_color('blue')
ax_loss.axis['right3'].label.set_color('orange')
ax_audio_score.axis['right4'].label.set_color('purple')

ax_bitrate.axis['left'].major_ticks.set_color('green')
ax_layers.axis['right'].major_ticks.set_color('red')
ax_delta.axis['right2'].major_ticks.set_color('blue')
ax_loss.axis['right3'].major_ticks.set_color('orange')
ax_audio_score.axis['right4'].major_ticks.set_color('purple')

ax_bitrate.axis['left'].major_ticklabels.set_color('green')
ax_layers.axis['right'].major_ticklabels.set_color('red')
ax_delta.axis['right2'].major_ticklabels.set_color('blue')
ax_loss.axis['right3'].major_ticklabels.set_color('orange')
ax_audio_score.axis['right4'].major_ticklabels.set_color('purple')

ax_bitrate.axis['left'].line.set_color('green')
ax_layers.axis['right'].line.set_color('red')
ax_delta.axis['right2'].line.set_color('blue')
ax_loss.axis['right3'].line.set_color('orange')
ax_audio_score.axis['right4'].line.set_color('purple')

#title
fig.suptitle(title, fontsize=16)

#plt.show()

#save png with tight mode#######################
plt.tight_layout()
plt.savefig("statistics.png",bbox_inches='tight')
# %%
