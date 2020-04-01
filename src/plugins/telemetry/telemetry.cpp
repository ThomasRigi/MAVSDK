// WARNING: THIS FILE IS AUTOGENERATED! As such, it should not be edited.
// Edits need to be made to the proto files
// (see https://github.com/mavlink/MAVSDK-Proto/blob/master/protos/telemetry/telemetry.proto)

#include "telemetry_impl.h"
#include "plugins/telemetry/telemetry.h"

namespace mavsdk {

Telemetry::Telemetry(System& system) : PluginBase(), _impl{new TelemetryImpl(system)} {}

Telemetry::~Telemetry() {}


void Telemetry::position_async(position_callback_t callback)
{
    _impl->position_async(callback);
}

Telemetry::Position Telemetry::position() const
{
    return _impl->position();
}

void Telemetry::home_async(home_callback_t callback)
{
    _impl->home_async(callback);
}

Telemetry::Position Telemetry::home() const
{
    return _impl->home();
}

void Telemetry::in_air_async(in_air_callback_t callback)
{
    _impl->in_air_async(callback);
}

bool Telemetry::in_air() const
{
    return _impl->in_air();
}

void Telemetry::landed_state_async(landed_state_callback_t callback)
{
    _impl->landed_state_async(callback);
}

Telemetry::LandedState Telemetry::landed_state() const
{
    return _impl->landed_state();
}

void Telemetry::armed_async(armed_callback_t callback)
{
    _impl->armed_async(callback);
}

bool Telemetry::armed() const
{
    return _impl->armed();
}

void Telemetry::attitude_quaternion_async(attitude_quaternion_callback_t callback)
{
    _impl->attitude_quaternion_async(callback);
}

Telemetry::Quaternion Telemetry::attitude_quaternion() const
{
    return _impl->attitude_quaternion();
}

void Telemetry::attitude_euler_async(attitude_euler_callback_t callback)
{
    _impl->attitude_euler_async(callback);
}

Telemetry::EulerAngle Telemetry::attitude_euler() const
{
    return _impl->attitude_euler();
}

void Telemetry::attitude_angular_velocity_body_async(attitude_angular_velocity_body_callback_t callback)
{
    _impl->attitude_angular_velocity_body_async(callback);
}

Telemetry::AngularVelocityBody Telemetry::attitude_angular_velocity_body() const
{
    return _impl->attitude_angular_velocity_body();
}

void Telemetry::camera_attitude_quaternion_async(camera_attitude_quaternion_callback_t callback)
{
    _impl->camera_attitude_quaternion_async(callback);
}

Telemetry::Quaternion Telemetry::camera_attitude_quaternion() const
{
    return _impl->camera_attitude_quaternion();
}

void Telemetry::camera_attitude_euler_async(camera_attitude_euler_callback_t callback)
{
    _impl->camera_attitude_euler_async(callback);
}

Telemetry::EulerAngle Telemetry::camera_attitude_euler() const
{
    return _impl->camera_attitude_euler();
}

void Telemetry::ground_speed_ned_async(ground_speed_ned_callback_t callback)
{
    _impl->ground_speed_ned_async(callback);
}

Telemetry::SpeedNed Telemetry::ground_speed_ned() const
{
    return _impl->ground_speed_ned();
}

void Telemetry::gps_info_async(gps_info_callback_t callback)
{
    _impl->gps_info_async(callback);
}

Telemetry::GpsInfo Telemetry::gps_info() const
{
    return _impl->gps_info();
}

void Telemetry::battery_async(battery_callback_t callback)
{
    _impl->battery_async(callback);
}

Telemetry::Battery Telemetry::battery() const
{
    return _impl->battery();
}

void Telemetry::flight_mode_async(flight_mode_callback_t callback)
{
    _impl->flight_mode_async(callback);
}

Telemetry::FlightMode Telemetry::flight_mode() const
{
    return _impl->flight_mode();
}

void Telemetry::health_async(health_callback_t callback)
{
    _impl->health_async(callback);
}

Telemetry::Health Telemetry::health() const
{
    return _impl->health();
}

void Telemetry::rc_status_async(rc_status_callback_t callback)
{
    _impl->rc_status_async(callback);
}

Telemetry::RcStatus Telemetry::rc_status() const
{
    return _impl->rc_status();
}

void Telemetry::status_text_async(status_text_callback_t callback)
{
    _impl->status_text_async(callback);
}

Telemetry::StatusText Telemetry::status_text() const
{
    return _impl->status_text();
}

void Telemetry::actuator_control_target_async(actuator_control_target_callback_t callback)
{
    _impl->actuator_control_target_async(callback);
}

Telemetry::ActuatorControlTarget Telemetry::actuator_control_target() const
{
    return _impl->actuator_control_target();
}

void Telemetry::actuator_output_status_async(actuator_output_status_callback_t callback)
{
    _impl->actuator_output_status_async(callback);
}

Telemetry::ActuatorOutputStatus Telemetry::actuator_output_status() const
{
    return _impl->actuator_output_status();
}

void Telemetry::odometry_async(odometry_callback_t callback)
{
    _impl->odometry_async(callback);
}

Telemetry::Odometry Telemetry::odometry() const
{
    return _impl->odometry();
}

void Telemetry::position_velocity_ned_async(position_velocity_ned_callback_t callback)
{
    _impl->position_velocity_ned_async(callback);
}

Telemetry::PositionVelocityNed Telemetry::position_velocity_ned() const
{
    return _impl->position_velocity_ned();
}

void Telemetry::ground_truth_async(ground_truth_callback_t callback)
{
    _impl->ground_truth_async(callback);
}

Telemetry::GroundTruth Telemetry::ground_truth() const
{
    return _impl->ground_truth();
}

void Telemetry::fixedwing_metrics_async(fixedwing_metrics_callback_t callback)
{
    _impl->fixedwing_metrics_async(callback);
}

Telemetry::FixedwingMetrics Telemetry::fixedwing_metrics() const
{
    return _impl->fixedwing_metrics();
}

void Telemetry::imu_async(imu_callback_t callback)
{
    _impl->imu_async(callback);
}

Telemetry::Imu Telemetry::imu() const
{
    return _impl->imu();
}

void Telemetry::health_all_ok_async(health_all_ok_callback_t callback)
{
    _impl->health_all_ok_async(callback);
}

bool Telemetry::health_all_ok() const
{
    return _impl->health_all_ok();
}

void Telemetry::unix_epoch_time_async(unix_epoch_time_callback_t callback)
{
    _impl->unix_epoch_time_async(callback);
}

uint64_t Telemetry::unix_epoch_time() const
{
    return _impl->unix_epoch_time();
}

void Telemetry::set_rate_position_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_position_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_position(double rate_hz) const
{
    return _impl->set_rate_position(rate_hz);
}

void Telemetry::set_rate_home_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_home_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_home(double rate_hz) const
{
    return _impl->set_rate_home(rate_hz);
}

void Telemetry::set_rate_in_air_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_in_air_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_in_air(double rate_hz) const
{
    return _impl->set_rate_in_air(rate_hz);
}

void Telemetry::set_rate_landed_state_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_landed_state_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_landed_state(double rate_hz) const
{
    return _impl->set_rate_landed_state(rate_hz);
}

void Telemetry::set_rate_attitude_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_attitude_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_attitude(double rate_hz) const
{
    return _impl->set_rate_attitude(rate_hz);
}

void Telemetry::set_rate_camera_attitude_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_camera_attitude_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_camera_attitude(double rate_hz) const
{
    return _impl->set_rate_camera_attitude(rate_hz);
}

void Telemetry::set_rate_ground_speed_ned_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_ground_speed_ned_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_ground_speed_ned(double rate_hz) const
{
    return _impl->set_rate_ground_speed_ned(rate_hz);
}

void Telemetry::set_rate_gps_info_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_gps_info_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_gps_info(double rate_hz) const
{
    return _impl->set_rate_gps_info(rate_hz);
}

void Telemetry::set_rate_battery_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_battery_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_battery(double rate_hz) const
{
    return _impl->set_rate_battery(rate_hz);
}

void Telemetry::set_rate_rc_status_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_rc_status_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_rc_status(double rate_hz) const
{
    return _impl->set_rate_rc_status(rate_hz);
}

void Telemetry::set_rate_actuator_control_target_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_actuator_control_target_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_actuator_control_target(double rate_hz) const
{
    return _impl->set_rate_actuator_control_target(rate_hz);
}

void Telemetry::set_rate_actuator_output_status_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_actuator_output_status_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_actuator_output_status(double rate_hz) const
{
    return _impl->set_rate_actuator_output_status(rate_hz);
}

void Telemetry::set_rate_odometry_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_odometry_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_odometry(double rate_hz) const
{
    return _impl->set_rate_odometry(rate_hz);
}

void Telemetry::set_rate_position_velocity_ned_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_position_velocity_ned_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_position_velocity_ned(double rate_hz) const
{
    return _impl->set_rate_position_velocity_ned(rate_hz);
}

void Telemetry::set_rate_ground_truth_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_ground_truth_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_ground_truth(double rate_hz) const
{
    return _impl->set_rate_ground_truth(rate_hz);
}

void Telemetry::set_rate_fixedwing_metrics_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_fixedwing_metrics_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_fixedwing_metrics(double rate_hz) const
{
    return _impl->set_rate_fixedwing_metrics(rate_hz);
}

void Telemetry::set_rate_imu_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_imu_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_imu(double rate_hz) const
{
    return _impl->set_rate_imu(rate_hz);
}

void Telemetry::set_rate_unix_epoch_time_async(double rate_hz, const result_callback_t callback)
{
    _impl->set_rate_unix_epoch_time_async(rate_hz, callback);
}

Telemetry::Result Telemetry::set_rate_unix_epoch_time(double rate_hz) const
{
    return _impl->set_rate_unix_epoch_time(rate_hz);
}


























































const char* Telemetry::result_str(Telemetry::Result result)
{
    switch (result) {
        case Telemetry::Result::Unknown:
            return "Unknown error";
        case Telemetry::Result::Success:
            return "Success: the telemetry command was accepted by the vehicle";
        case Telemetry::Result::NoSystem:
            return "No system connected";
        case Telemetry::Result::ConnectionError:
            return "Connection error";
        case Telemetry::Result::Busy:
            return "Vehicle is busy";
        case Telemetry::Result::CommandDenied:
            return "Command refused by vehicle";
        case Telemetry::Result::Timeout:
            return "Request timed out";
        default:
            return "Unknown";
    }
}




} // namespace mavsdk