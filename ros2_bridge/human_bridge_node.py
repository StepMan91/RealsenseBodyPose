#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from visualization_msgs.msg import Marker, MarkerArray
from geometry_msgs.msg import Point
import socket
import json
import threading

class HumanBridgeNode(Node):
    def __init__(self):
        super().__init__('human_bridge_node')
        self.publisher_ = self.create_publisher(MarkerArray, 'human_skeleton', 10)
        self.declare_parameter('udp_port', 8888)
        self.port = self.get_parameter('udp_port').value
        
        self.get_logger().info(f'Starting UDP Bridge on port {self.port}...')
        
        # Start UDP listener in a separate thread
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('0.0.0.0', self.port))
        self.running = True
        self.thread = threading.Thread(target=self.udp_listener)
        self.thread.daemon = True
        self.thread.start()

    def udp_listener(self):
        while self.running and rclpy.ok():
            try:
                data, addr = self.sock.recvfrom(4096)
                json_str = data.decode('utf-8')
                self.process_data(json_str)
            except Exception as e:
                self.get_logger().error(f'UDP Error: {e}')

    def process_data(self, json_str):
        try:
            data = json.loads(json_str)
            skeletons = data.get('skeletons', [])
            
            marker_array = MarkerArray()
            
            for skel in skeletons:
                skel_id = skel['id']
                joints = skel['joints']
                
                # Create a sphere marker for each joint
                for joint_name, joint_data in joints.items():
                    marker = Marker()
                    marker.header.frame_id = "camera_link" # Adjust frame as needed
                    marker.header.stamp = self.get_clock().now().to_msg()
                    marker.ns = f"person_{skel_id}"
                    
                    # Map joint name to ID for stable tracking
                    # Simple hash or lookup
                    marker.id = skel_id * 100 + abs(hash(joint_name)) % 100
                    
                    marker.type = Marker.SPHERE
                    marker.action = Marker.ADD
                    
                    # Coordinate transform (Camera -> ROS)
                    # Camera: X=Right, Y=Down, Z=Forward
                    # ROS: X=Forward, Y=Left, Z=Up
                    # Simple mapping: ROS_X = Cam_Z, ROS_Y = -Cam_X, ROS_Z = -Cam_Y (approx)
                    # Or keep as is and use TF
                    
                    # Let's keep raw first (visualize in Rviz relative to camera)
                    marker.pose.position.x = joint_data['z'] # Depth becomes X (forward)
                    marker.pose.position.y = -joint_data['x'] # Right becomes -Y (left)
                    marker.pose.position.z = -joint_data['y'] # Down becomes -Z (up, if 0 is head)
                    # Note: Y is usually 'down' in camera, so 'up' is -Y.
                    
                    marker.pose.orientation.w = 1.0
                    
                    marker.scale.x = 0.05
                    marker.scale.y = 0.05
                    marker.scale.z = 0.05
                    
                    marker.color.a = 1.0
                    marker.color.r = 1.0 if joint_name.endswith("Wrist") else 0.0
                    marker.color.g = 1.0
                    marker.color.b = 0.0
                    
                    marker_array.markers.append(marker)
            
            self.publisher_.publish(marker_array)
            # self.get_logger().info(f'Published {len(marker_array.markers)} joints')

        except json.JSONDecodeError:
            pass

    def destroy_node(self):
        self.running = False
        self.sock.close()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = HumanBridgeNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
