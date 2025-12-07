#!/usr/bin/env python3
"""
CSV to ROS2 Bag Converter for RealSense Body Pose Data
Usage: python3 csv_to_rosbag.py <input_csv_file> [output_bag_name]
"""

import sys
import csv
import time
import argparse
import json
from pathlib import Path

# ROS2 imports
try:
    import rclpy
    from rclpy.time import Time
    from rclpy.serialization import serialize_message
    from visualization_msgs.msg import Marker, MarkerArray
    from std_msgs.msg import String
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

    # Topic 1: Visualization Markers
    topic_markers = '/human_skeleton'
    create_topic(writer, topic_markers, 'visualization_msgs/msg/MarkerArray')

    # Topic 2: Raw JSON (simulating UDP stream)
    topic_json = '/human_json'
    create_topic(writer, topic_json, 'std_msgs/msg/String')

    # Mapping from CSV joint indices to Names (matches Utils.h / DataRecorder.cpp)
    # The CSV now has named columns: Nose_X, LeftShoulder_X, etc.
    JOINT_NAMES = [
        "Nose", "LeftEye", "RightEye", "LeftEar", "RightEar", 
        "LeftShoulder", "RightShoulder", "LeftElbow", "RightElbow", 
        "LeftWrist", "RightWrist", "LeftHip", "RightHip", 
        "LeftKnee", "RightKnee", "LeftAnkle", "RightAnkle"
    ]

    count = 0
    
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
                process_frame(writer, topic_markers, topic_json, current_timestamp, current_frame_skeletons, JOINT_NAMES)
                count += 1
                
                # Reset for new frame
                current_timestamp = ts
                current_frame_skeletons = []
            
            current_frame_skeletons.append(row)
            
        # Process last frame
        if current_frame_skeletons:
            process_frame(writer, topic_markers, topic_json, current_timestamp, current_frame_skeletons, JOINT_NAMES)
            count += 1

    print(f"Done! Written {count} messages to {bag_path}")

def format_float(val):
    return round(val, 3)

def process_frame(writer, topic_markers, topic_json, timestamp_ms, rows, joint_names):
    marker_array = MarkerArray()
    
    timestamp_ns = timestamp_ms * 1_000_000
    ros_time = Time(seconds=timestamp_ns // 1_000_000_000, nanoseconds=timestamp_ns % 1_000_000_000)
    
    # ---------------------------------------------------------
    # 1. Build JSON Object (replicating UdpSender.cpp logic)
    # ---------------------------------------------------------
    json_data = {"skeletons": []}

    for row_idx, row in enumerate(rows):
        person_id = int(row['PersonID'])
        # UDP uses 1-based ID usually, or just row index. 
        # DataRecorder saves row index as PersonID if not tracked.
        # Let's use PersonID + 1 for consistency with UdpSender update 
        # (UdpSender uses i+1).
        
        skel_obj = {
            "id": person_id + 1,
            "joints": {}
        }

        # Handle Pelvis (Stored explicitly in CSV now)
        try:
            px = float(row['Pelvis_X'])
            py = float(row['Pelvis_Y'])
            pz = float(row['Pelvis_Z'])
            pc = float(row['Pelvis_Confidence'])
            
            if pc > 0.3 and pz > 0:
                 skel_obj["joints"]["Pelvis"] = {
                     "x": format_float(px),
                     "y": format_float(py),
                     "z": format_float(pz),
                     "confidence": format_float(pc)
                 }
        except KeyError:
            pass # Pelvis might not be in older CSVs, but code was just updated.

        # Handle Standard Joints
        for name in joint_names:
            try:
                x = float(row[f'{name}_X'])
                y = float(row[f'{name}_Y'])
                z = float(row[f'{name}_Z'])
                conf = float(row[f'{name}_Confidence']) # Note: DataRecorder uses _Confidence now
            except KeyError:
                # Fallback for Mixed/Legacy CSVs (though user just recompiled)
                continue

            if conf > 0.3 and z > 0:
                skel_obj["joints"][name] = {
                    "x": format_float(x), 
                    "y": format_float(y), 
                    "z": format_float(z), 
                    "confidence": format_float(conf)
                }
                
                # -------------------------------------------------
                # 2. Add Marker for Visualization
                # -------------------------------------------------
                marker = Marker()
                marker.header.frame_id = "camera_link"
                marker.header.stamp = ros_time.to_msg()
                marker.ns = f"person_{person_id}"
                marker.id = person_id * 100 + abs(hash(name)) % 1000
                marker.type = Marker.SPHERE
                marker.action = Marker.ADD
                
                # Coordinate Transform (matches human_bridge_node.py)
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
                    marker.color.g = 0.0
                    marker.color.b = 0.0
                else:
                    marker.color.r = 0.0
                    marker.color.g = 1.0
                    marker.color.b = 0.0
                    
                marker_array.markers.append(marker)
        
        json_data["skeletons"].append(skel_obj)

    # Write Markers
    writer.write(topic_markers, serialize_message(marker_array), timestamp_ns)

    # Write JSON
    json_msg = String()
    json_msg.data = json.dumps(json_data)
    writer.write(topic_json, serialize_message(json_msg), timestamp_ns)

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
