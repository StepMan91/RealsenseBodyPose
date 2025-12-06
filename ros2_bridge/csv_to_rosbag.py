#!/usr/bin/env python3
"""
CSV to ROS2 Bag Converter for RealSense Body Pose Data
Usage: python3 csv_to_rosbag.py <input_csv_file> [output_bag_name]
"""

import sys
import csv
import time
import argparse
from pathlib import Path

# ROS2 imports
try:
    import rclpy
    from rclpy.time import Time
    from rclpy.serialization import serialize_message
    from visualization_msgs.msg import Marker, MarkerArray
    import rosbag2_py
except ImportError:
    print("Error: ROS2 python packages not found. Please source your ROS2 environment.")
    sys.exit(1)

def get_rosbag_options(path, serialization_format='cdr'):
    storage_options = rosbag2_py.StorageOptions(
        uri=path,
        storage_id='sqlite3')
    converter_options = rosbag2_py.ConverterOptions(
        input_serialization_format=serialization_format,
        output_serialization_format=serialization_format)
    return storage_options, converter_options

def create_topic(writer, topic_name, topic_type, serialization_format='cdr'):
    topic_info = rosbag2_py.TopicMetadata(
        name=topic_name,
        type=topic_type,
        serialization_format=serialization_format)
    writer.create_topic(topic_info)

def convert_csv_to_bag(csv_path, bag_path):
    print(f"Converting {csv_path} to {bag_path}...")
    
    writer = rosbag2_py.SequentialWriter()
    storage_options, converter_options = get_rosbag_options(bag_path)
    writer.open(storage_options, converter_options)

    topic_name = '/human_skeleton'
    create_topic(writer, topic_name, 'visualization_msgs/msg/MarkerArray')

    # Mapping from CSV joint indices to Names (matches Utils.h)
    JOINT_NAMES = [
        "Nose", "Left Eye", "Right Eye", "Left Ear", "Right Ear", 
        "Left Shoulder", "Right Shoulder", "Left Elbow", "Right Elbow", 
        "Left Wrist", "Right Wrist", "Left Hip", "Right Hip", 
        "Left Knee", "Right Knee", "Left Ankle", "Right Ankle"
    ]

    count = 0
    start_time_ns = -1
    
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        
        # Group rows by Timestamp
        current_timestamp = None
        current_frame_skeletons = []
        
        for row in reader:
            ts = int(row['Timestamp']) # ms
            
            if current_timestamp is None:
                current_timestamp = ts
            
            if ts != current_timestamp:
                # Process previous frame
                process_frame(writer, topic_name, current_timestamp, current_frame_skeletons, JOINT_NAMES)
                count += 1
                
                # Reset for new frame
                current_timestamp = ts
                current_frame_skeletons = []
            
            current_frame_skeletons.append(row)
            
        # Process last frame
        if current_frame_skeletons:
            process_frame(writer, topic_name, current_timestamp, current_frame_skeletons, JOINT_NAMES)
            count += 1

    print(f"Done! Written {count} messages to {bag_path}")

def process_frame(writer, topic_name, timestamp_ms, rows, joint_names):
    marker_array = MarkerArray()
    
    # ROS2 Time (ns)
    # We map the Recording start to "now" or keep relative time? 
    # Usually better to use the actual timestamp if it's absolute, or relative.
    # The C++ app saved system time in ms.
    
    timestamp_ns = timestamp_ms * 1_000_000
    ros_time = Time(seconds=timestamp_ns // 1_000_000_000, nanoseconds=timestamp_ns % 1_000_000_000)
    
    for row in rows:
        person_id = int(row['PersonID'])
        
        for i, name in enumerate(joint_names):
            x = float(row[f'J{i}_X'])
            y = float(row[f'J{i}_Y'])
            z = float(row[f'J{i}_Z'])
            conf = float(row[f'J{i}_Conf'])
            
            if conf < 0.3 or z == 0:
                continue
                
            marker = Marker()
            marker.header.frame_id = "camera_link"
            marker.header.stamp = ros_time.to_msg()
            marker.ns = f"person_{person_id}"
            marker.id = person_id * 100 + i
            marker.type = Marker.SPHERE
            marker.action = Marker.ADD
            
            # Coordinate Transform (matches human_bridge_node.py)
            # ROS_X = Cam_Z (Forward)
            # ROS_Y = -Cam_X (Left)
            # ROS_Z = -Cam_Y (Up)
            marker.pose.position.x = z
            marker.pose.position.y = -x
            marker.pose.position.z = -y
            
            marker.pose.orientation.w = 1.0
            marker.scale.x = 0.05
            marker.scale.y = 0.05
            marker.scale.z = 0.05
            
            marker.color.a = 1.0
            if "Wrist" in name:
                marker.color.r = 1.0
            else:
                marker.color.g = 1.0
                
            marker_array.markers.append(marker)
            
    writer.write(topic_name, serialize_message(marker_array), timestamp_ns)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert CSV Skeleton Data to ROS2 Bag")
    parser.add_argument("input", help="Path to input CSV file")
    parser.add_argument("output", nargs="?", help="Path to output bag directory (optional)")
    
    args = parser.parse_args()
    
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Error: File {input_path} does not exist.")
        sys.exit(1)
        
    if args.output:
        output_path = args.output
    else:
        output_path = f"rosbag_{input_path.stem}"
        
    convert_csv_to_bag(str(input_path), output_path)
