#!/usr/bin/env python3

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import TimerAction

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='ros2_xeno',
            executable='xeno_node',
            name='xeno_node',
            output='screen',
        ),
        TimerAction(
            period=1.0,  # Adjust the delay in seconds as needed
            actions=[
                Node(
                    package='ros2_xeno',
                    executable='xeno_node2',
                    name='xeno_node2',
                    output='screen',
                ),
            ],
        ),
    ])

