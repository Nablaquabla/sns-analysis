#!/bin/bash
remDir="/SNS/lustre/FD1/IPTS-9963/shared/SBIMA1/Data/csi/sns_data/$1"
locDir="/var/phy/project/phil/grayson/COHERENT/CsI/sns_data/"
rsync -avzhP sns:$remDir $locDir