import json;
import numpy;
import math;
import h5py;
import hdf5_explorer;

dataPath = '/groups/flyem/data/medulla-FIB-Z1211-25-production/align2/stitched_layers/1-12_1500-7499_20130615T143300';

dataFile = open(dataPath + '/seg-0.3-6b63839cd45e2c9566ab4d5804c796de-v1.json');

data = json.load(dataFile);

for key in data.keys():
    print(key);

print('\n');

dataFile.close();

volume = data['subvolumes'][1];

dataFile = open(volume['config-file']);
data = json.load(dataFile);
for key in data.keys():
    print(key);

dataFile.close();

print(data['subvolumes'][0]['segmentation-file']);


hdf5_explorer.print_hdf5_file_structure(data['subvolumes'][0]['segmentation-file'])

f = h5py.File(data['subvolumes'][0]['segmentation-file'], 'r');
stack = f['/stack'];
print(stack[0].shape);

hdf5_explorer.plotImage(stack[0]);
