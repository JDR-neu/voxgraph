//
// Created by victor on 19.12.18.
//

#include "voxgraph/tools/visualization/submap_visuals.h"
#include <cblox/mesh/submap_mesher.h>
#include <eigen_conversions/eigen_msg.h>
#include <voxblox_ros/mesh_vis.h>
#include <voxblox_ros/ptcloud_vis.h>
#include <memory>
#include <string>

namespace voxgraph {
SubmapVisuals::SubmapVisuals(const VoxgraphSubmap::Config &submap_config) {
  // TODO(victorr): Read this from ROS params
  mesh_config_.min_weight = 2;
  submap_mesher_.reset(new cblox::SubmapMesher(submap_config, mesh_config_));
}

void SubmapVisuals::publishMesh(const voxblox::MeshLayer::Ptr &mesh_layer_ptr,
                                const std::string &submap_frame,
                                const ros::Publisher &publisher,
                                const voxblox::ColorMode &color_mode) const {
  // Create a marker containing the mesh
  visualization_msgs::Marker marker;
  voxblox::fillMarkerWithMesh(mesh_layer_ptr, color_mode, &marker);
  marker.header.frame_id = submap_frame;
  // Update the marker's transform each time its TF frame is updated:
  marker.frame_locked = true;
  publisher.publish(marker);
}

void SubmapVisuals::publishMesh(
    const cblox::SubmapCollection<VoxgraphSubmap> &submap_collection,
    const cblox::SubmapID &submap_id, const voxblox::Color &submap_color,
    const std::string &submap_frame, const ros::Publisher &publisher) const {
  // Get a pointer to the submap
  VoxgraphSubmap::ConstPtr submap_ptr =
      submap_collection.getSubMapConstPtrById(submap_id);
  CHECK_NOTNULL(submap_ptr);

  // Generate the mesh
  auto mesh_layer_ptr =
      std::make_shared<cblox::MeshLayer>(submap_collection.block_size());

  voxblox::MeshIntegrator<voxblox::TsdfVoxel> reference_mesh_integrator(
      mesh_config_, submap_ptr->getTsdfMap().getTsdfLayer(),
      mesh_layer_ptr.get());
  reference_mesh_integrator.generateMesh(false, false);
  submap_mesher_->colorMeshLayer(submap_color, mesh_layer_ptr.get());

  // Publish mesh
  publishMesh(mesh_layer_ptr, submap_frame, publisher);
}

void SubmapVisuals::publishSeparatedMesh(
    const cblox::SubmapCollection<VoxgraphSubmap> &submap_collection,
    const std::string &world_frame, const ros::Publisher &publisher) {
  auto mesh_layer_ptr =
      std::make_shared<cblox::MeshLayer>(submap_collection.block_size());
  submap_mesher_->generateSeparatedMesh(submap_collection,
                                        mesh_layer_ptr.get());
  publishMesh(mesh_layer_ptr, world_frame, publisher);
}

void SubmapVisuals::publishCombinedMesh(
    const cblox::SubmapCollection<VoxgraphSubmap> &submap_collection,
    const std::string &world_frame, const ros::Publisher &publisher) {
  auto mesh_layer_ptr =
      std::make_shared<cblox::MeshLayer>(submap_collection.block_size());
  submap_mesher_->generateCombinedMesh(submap_collection, mesh_layer_ptr.get());
  publishMesh(mesh_layer_ptr, world_frame, publisher,
              voxblox::ColorMode::kNormals);
}

void SubmapVisuals::publishBox(
    const VoxgraphSubmap::BoxCornerMatrix &box_corner_matrix,
    const voxblox::Color &box_color, const std::string &frame_id,
    const std::string &name_space, const ros::Publisher &publisher) const {
  // Create and configure the marker
  visualization_msgs::Marker marker;
  marker.header.frame_id = frame_id;
  marker.header.stamp = ros::Time();
  marker.ns = name_space;
  marker.id = 0;
  marker.type = visualization_msgs::Marker::LINE_LIST;
  marker.action = visualization_msgs::Marker::ADD;
  marker.pose.orientation.w = 1.0;  // Set to unit quaternion
  marker.scale.x = 0.1;
  voxblox::colorVoxbloxToMsg(box_color, &marker.color);
  marker.frame_locked = false;

  // Add the box edges to the marker's
  std::bitset<3> endpoint_a_idx, endpoint_b_idx;
  for (unsigned int i = 0; i < 8; i++) {
    endpoint_a_idx = std::bitset<3>(i);
    for (unsigned int j = i + 1; j < 8; j++) {
      endpoint_b_idx = std::bitset<3>(j);
      // The endpoints describe an edge of the box if the
      // Hamming distance between both endpoint indices is 1
      if ((endpoint_a_idx ^ endpoint_b_idx).count() == 1) {
        // Add the edge's endpoints to the marker's line list
        geometry_msgs::Point point_msg;
        tf::pointEigenToMsg(box_corner_matrix.col(i).cast<double>(), point_msg);
        marker.points.push_back(point_msg);
        tf::pointEigenToMsg(box_corner_matrix.col(j).cast<double>(), point_msg);
        marker.points.push_back(point_msg);
      }
    }
  }

  // Publish the visualization
  publisher.publish(marker);
}

// TODO(victorr): Implement TSDF visualization

}  // namespace voxgraph