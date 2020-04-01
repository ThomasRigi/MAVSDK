#include "telemetry_impl.h"
#include "system.h"
#include "math_conversions.h"
#include "global_include.h"
#include <cmath>
#include <functional>
#include <string>
#include <array>
#include <cassert>

namespace mavsdk {

TelemetryImpl::TelemetryImpl(System& system) : PluginImplBase(system)
{
    _parent->register_plugin(this);
}

TelemetryImpl::~TelemetryImpl()
{
    _parent->unregister_plugin(this);
}

void TelemetryImpl::init()
{
    using namespace std::placeholders; // for `_1`

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_LOCAL_POSITION_NED,
        std::bind(&TelemetryImpl::process_position_velocity_ned, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_GLOBAL_POSITION_INT,
        std::bind(&TelemetryImpl::process_global_position_int, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_HOME_POSITION,
        std::bind(&TelemetryImpl::process_home_position, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_ATTITUDE, std::bind(&TelemetryImpl::process_attitude, this, _1), this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_ATTITUDE_QUATERNION,
        std::bind(&TelemetryImpl::process_attitude_quaternion, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_MOUNT_ORIENTATION,
        std::bind(&TelemetryImpl::process_mount_orientation, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_GPS_RAW_INT, std::bind(&TelemetryImpl::process_gps_raw_int, this, _1), this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_EXTENDED_SYS_STATE,
        std::bind(&TelemetryImpl::process_extended_sys_state, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_SYS_STATUS, std::bind(&TelemetryImpl::process_sys_status, this, _1), this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_HEARTBEAT, std::bind(&TelemetryImpl::process_heartbeat, this, _1), this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_STATUSTEXT, std::bind(&TelemetryImpl::process_statustext, this, _1), this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_RC_CHANNELS, std::bind(&TelemetryImpl::process_rc_channels, this, _1), this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_ACTUATOR_CONTROL_TARGET,
        std::bind(&TelemetryImpl::process_actuator_control_target, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_ACTUATOR_OUTPUT_STATUS,
        std::bind(&TelemetryImpl::process_actuator_output_status, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_ODOMETRY, std::bind(&TelemetryImpl::process_odometry, this, _1), this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_UTM_GLOBAL_POSITION,
        std::bind(&TelemetryImpl::process_unix_epoch_time, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_HIGHRES_IMU,
        std::bind(&TelemetryImpl::process_imu_reading_ned, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_VFR_HUD,
        std::bind(&TelemetryImpl::process_fixedwing_metrics, this, _1),
        this);

    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_HIL_STATE_QUATERNION,
        std::bind(&TelemetryImpl::process_ground_truth, this, _1),
        this);

    _parent->register_param_changed_handler(
        std::bind(&TelemetryImpl::process_parameter_update, this, _1), this);
}

void TelemetryImpl::deinit()
{
    _parent->unregister_timeout_handler(_rc_channels_timeout_cookie);
    _parent->unregister_timeout_handler(_gps_raw_timeout_cookie);
    _parent->unregister_timeout_handler(_unix_epoch_timeout_cookie);
    _parent->unregister_param_changed_handler(this);
    _parent->unregister_all_mavlink_message_handlers(this);
}

void TelemetryImpl::enable()
{
    _parent->register_timeout_handler(
        std::bind(&TelemetryImpl::receive_rc_channels_timeout, this),
        1.0,
        &_rc_channels_timeout_cookie);

    _parent->register_timeout_handler(
        std::bind(&TelemetryImpl::receive_gps_raw_timeout, this), 2.0, &_gps_raw_timeout_cookie);

    _parent->register_timeout_handler(
        std::bind(&TelemetryImpl::receive_unix_epoch_timeout, this),
        2.0,
        &_unix_epoch_timeout_cookie);

    // FIXME: The calibration check should eventually be better than this.
    //        For now, we just do the same as QGC does.

    _parent->get_param_int_async(
        std::string("CAL_GYRO0_ID"),
        std::bind(
            &TelemetryImpl::receive_param_cal_gyro,
            this,
            std::placeholders::_1,
            std::placeholders::_2),
        this);

    _parent->get_param_int_async(
        std::string("CAL_ACC0_ID"),
        std::bind(
            &TelemetryImpl::receive_param_cal_accel,
            this,
            std::placeholders::_1,
            std::placeholders::_2),
        this);

    _parent->get_param_int_async(
        std::string("CAL_MAG0_ID"),
        std::bind(
            &TelemetryImpl::receive_param_cal_mag,
            this,
            std::placeholders::_1,
            std::placeholders::_2),
        this);

#ifdef LEVEL_CALIBRATION
    _parent->param_float_async(
        std::string("SENS_BOARD_X_OFF"),
        std::bind(
            &TelemetryImpl::receive_param_cal_level,
            this,
            std::placeholders::_1,
            std::placeholders::_2),
        this);
#else
    // If not available, just hardcode it to true.
    set_health_level_calibration(true);
#endif

    _parent->get_param_int_async(
        std::string("SYS_HITL"),
        std::bind(
            &TelemetryImpl::receive_param_hitl, this, std::placeholders::_1, std::placeholders::_2),
        this);
}

void TelemetryImpl::disable() {}

Telemetry::Result TelemetryImpl::set_rate_position_velocity_ned(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_LOCAL_POSITION_NED, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_position(double rate_hz)
{
    _position_rate_hz = rate_hz;
    double max_rate_hz = std::max(_position_rate_hz, _ground_speed_ned_rate_hz);

    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, max_rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_home(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_HOME_POSITION, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_in_air(double rate_hz)
{
    return set_rate_landed_state(rate_hz);
}

Telemetry::Result TelemetryImpl::set_rate_landed_state(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_EXTENDED_SYS_STATE, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_attitude(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_ATTITUDE_QUATERNION, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_camera_attitude(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_MOUNT_ORIENTATION, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_ground_speed_ned(double rate_hz)
{
    _ground_speed_ned_rate_hz = rate_hz;
    double max_rate_hz = std::max(_position_rate_hz, _ground_speed_ned_rate_hz);

    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, max_rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_imu(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_HIGHRES_IMU, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_fixedwing_metrics(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_VFR_HUD, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_ground_truth(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_HIL_STATE_QUATERNION, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_gps_info(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_GPS_RAW_INT, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_battery(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_SYS_STATUS, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_rc_status(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_RC_CHANNELS, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_actuator_control_target(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_ACTUATOR_CONTROL_TARGET, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_actuator_output_status(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_ACTUATOR_OUTPUT_STATUS, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_odometry(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_ODOMETRY, rate_hz));
}

Telemetry::Result TelemetryImpl::set_rate_unix_epoch_time(double rate_hz)
{
    return telemetry_result_from_command_result(
        _parent->set_msg_rate(MAVLINK_MSG_ID_UTM_GLOBAL_POSITION, rate_hz));
}

void TelemetryImpl::set_rate_position_velocity_ned_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_LOCAL_POSITION_NED,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_position_async(double rate_hz, Telemetry::result_callback_t callback)
{
    _position_rate_hz = rate_hz;
    double max_rate_hz = std::max(_position_rate_hz, _ground_speed_ned_rate_hz);

    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_GLOBAL_POSITION_INT,
        max_rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_home_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_HOME_POSITION,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_in_air_async(double rate_hz, Telemetry::result_callback_t callback)
{
    set_rate_landed_state_async(rate_hz, callback);
}

void TelemetryImpl::set_rate_landed_state_async(double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_EXTENDED_SYS_STATE,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_attitude_async(double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_ATTITUDE_QUATERNION,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_camera_attitude_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_MOUNT_ORIENTATION,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_ground_speed_ned_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _ground_speed_ned_rate_hz = rate_hz;
    double max_rate_hz = std::max(_position_rate_hz, _ground_speed_ned_rate_hz);

    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_GLOBAL_POSITION_INT,
        max_rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_imu_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_HIGHRES_IMU,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_fixedwing_metrics_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_VFR_HUD,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_ground_truth_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_HIL_STATE_QUATERNION,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_gps_info_async(double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_GPS_RAW_INT,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_battery_async(double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_SYS_STATUS,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_rc_status_async(double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_RC_CHANNELS,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_unix_epoch_time_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_UTM_GLOBAL_POSITION,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_actuator_control_target_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_ACTUATOR_CONTROL_TARGET,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_actuator_output_status_async(
    double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_ACTUATOR_OUTPUT_STATUS,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

void TelemetryImpl::set_rate_odometry_async(double rate_hz, Telemetry::result_callback_t callback)
{
    _parent->set_msg_rate_async(
        MAVLINK_MSG_ID_ODOMETRY,
        rate_hz,
        std::bind(&TelemetryImpl::command_result_callback, std::placeholders::_1, callback));
}

Telemetry::Result
TelemetryImpl::telemetry_result_from_command_result(MAVLinkCommands::Result command_result)
{
    switch (command_result) {
        case MAVLinkCommands::Result::SUCCESS:
            return Telemetry::Result::Success;
        case MAVLinkCommands::Result::NO_SYSTEM:
            return Telemetry::Result::NoSystem;
        case MAVLinkCommands::Result::CONNECTION_ERROR:
            return Telemetry::Result::ConnectionError;
        case MAVLinkCommands::Result::BUSY:
            return Telemetry::Result::Busy;
        case MAVLinkCommands::Result::COMMAND_DENIED:
            return Telemetry::Result::CommandDenied;
        case MAVLinkCommands::Result::TIMEOUT:
            return Telemetry::Result::Timeout;
        default:
            return Telemetry::Result::Unknown;
    }
}

void TelemetryImpl::command_result_callback(
    MAVLinkCommands::Result command_result, const Telemetry::result_callback_t& callback)
{
    Telemetry::Result action_result = telemetry_result_from_command_result(command_result);

    callback(action_result);
}

void TelemetryImpl::process_position_velocity_ned(const mavlink_message_t& message)
{
    mavlink_local_position_ned_t local_position;
    mavlink_msg_local_position_ned_decode(&message, &local_position);
    set_position_velocity_ned(Telemetry::PositionVelocityNed({local_position.x,
                                                              local_position.y,
                                                              local_position.z,
                                                              local_position.vx,
                                                              local_position.vy,
                                                              local_position.vz}));

    if (_position_velocity_ned_subscription) {
        auto callback = _position_velocity_ned_subscription;
        auto arg = position_velocity_ned();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_global_position_int(const mavlink_message_t& message)
{
    mavlink_global_position_int_t global_position_int;
    mavlink_msg_global_position_int_decode(&message, &global_position_int);
    set_position(Telemetry::Position({global_position_int.lat * 1e-7,
                                      global_position_int.lon * 1e-7,
                                      global_position_int.alt * 1e-3f,
                                      global_position_int.relative_alt * 1e-3f}));
    set_ground_speed_ned({global_position_int.vx * 1e-2f,
                          global_position_int.vy * 1e-2f,
                          global_position_int.vz * 1e-2f});

    if (_position_subscription) {
        auto callback = _position_subscription;
        auto arg = position();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    if (_ground_speed_ned_subscription) {
        auto callback = _ground_speed_ned_subscription;
        auto arg = ground_speed_ned();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_home_position(const mavlink_message_t& message)
{
    mavlink_home_position_t home_position;
    mavlink_msg_home_position_decode(&message, &home_position);
    set_home_position(Telemetry::Position({home_position.latitude * 1e-7,
                                           home_position.longitude * 1e-7,
                                           home_position.altitude * 1e-3f,
                                           // the relative altitude of home is 0 by definition.
                                           0.0f}));

    set_health_home_position(true);

    if (_home_position_subscription) {
        auto callback = _home_position_subscription;
        auto arg = home();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_attitude(const mavlink_message_t& message)
{
    mavlink_attitude_t attitude;
    mavlink_msg_attitude_decode(&message, &attitude);

    Telemetry::EulerAngle euler_angle{attitude.roll, attitude.pitch, attitude.yaw};

    Telemetry::AngularVelocityBody angular_velocity_body{
        attitude.rollspeed, attitude.pitchspeed, attitude.yawspeed};
    set_attitude_angular_velocity_body(angular_velocity_body);

    auto quaternion = mavsdk::to_quaternion_from_euler_angle(euler_angle);
    set_attitude_quaternion(quaternion);

    if (_attitude_quaternion_angle_subscription) {
        auto callback = _attitude_quaternion_angle_subscription;
        auto arg = attitude_quaternion();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    if (_attitude_euler_angle_subscription) {
        auto callback = _attitude_euler_angle_subscription;
        auto arg = attitude_euler();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    if (_attitude_angular_velocity_body_subscription) {
        auto callback = _attitude_angular_velocity_body_subscription;
        auto arg = attitude_angular_velocity_body();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_attitude_quaternion(const mavlink_message_t& message)
{
    mavlink_attitude_quaternion_t mavlink_attitude_quaternion;
    mavlink_msg_attitude_quaternion_decode(&message, &mavlink_attitude_quaternion);

    Telemetry::Quaternion quaternion{mavlink_attitude_quaternion.q1,
                                     mavlink_attitude_quaternion.q2,
                                     mavlink_attitude_quaternion.q3,
                                     mavlink_attitude_quaternion.q4};

    Telemetry::AngularVelocityBody angular_velocity_body{mavlink_attitude_quaternion.rollspeed,
                                                         mavlink_attitude_quaternion.pitchspeed,
                                                         mavlink_attitude_quaternion.yawspeed};

    set_attitude_quaternion(quaternion);

    set_attitude_angular_velocity_body(angular_velocity_body);

    if (_attitude_quaternion_angle_subscription) {
        auto callback = _attitude_quaternion_angle_subscription;
        auto arg = attitude_quaternion();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    if (_attitude_euler_angle_subscription) {
        auto callback = _attitude_euler_angle_subscription;
        auto arg = attitude_euler();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    if (_attitude_angular_velocity_body_subscription) {
        auto callback = _attitude_angular_velocity_body_subscription;
        auto arg = attitude_angular_velocity_body();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_mount_orientation(const mavlink_message_t& message)
{
    mavlink_mount_orientation_t mount_orientation;
    mavlink_msg_mount_orientation_decode(&message, &mount_orientation);

    Telemetry::EulerAngle euler_angle{
        mount_orientation.roll, mount_orientation.pitch, mount_orientation.yaw_absolute};

    set_camera_attitude_euler_angle(euler_angle);

    if (_camera_attitude_quaternion_subscription) {
        auto callback = _camera_attitude_quaternion_subscription;
        auto arg = camera_attitude_quaternion();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    if (_camera_attitude_euler_angle_subscription) {
        auto callback = _camera_attitude_euler_angle_subscription;
        auto arg = camera_attitude_euler();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_imu_reading_ned(const mavlink_message_t& message)
{
    mavlink_highres_imu_t highres_imu;
    mavlink_msg_highres_imu_decode(&message, &highres_imu);
    set_imu_reading_ned(Telemetry::Imu({highres_imu.xacc,
                                        highres_imu.yacc,
                                        highres_imu.zacc,
                                        highres_imu.xgyro,
                                        highres_imu.ygyro,
                                        highres_imu.zgyro,
                                        highres_imu.xmag,
                                        highres_imu.ymag,
                                        highres_imu.zmag,
                                        highres_imu.temperature}));

    if (_imu_reading_ned_subscription) {
        auto callback = _imu_reading_ned_subscription;
        auto arg = imu();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_gps_raw_int(const mavlink_message_t& message)
{
    mavlink_gps_raw_int_t gps_raw_int;
    mavlink_msg_gps_raw_int_decode(&message, &gps_raw_int);

    Telemetry::FixType fix_type;
    switch (gps_raw_int.fix_type) {
        case 0:
            fix_type = Telemetry::FixType::NoGps;
            break;
        case 1:
            fix_type = Telemetry::FixType::NoFix;
            break;
        case 2:
            fix_type = Telemetry::FixType::Fix2D;
            break;
        case 3:
            fix_type = Telemetry::FixType::Fix3D;
            break;
        case 4:
            fix_type = Telemetry::FixType::FixDgps;
            break;
        case 5:
            fix_type = Telemetry::FixType::RtkFloat;
            break;
        case 6:
            fix_type = Telemetry::FixType::RtkFixed;
            break;
    }

    set_gps_info({gps_raw_int.satellites_visible, fix_type});

    // TODO: This is just an interim hack, we will have to look at
    //       estimator flags in order to decide if the position
    //       estimate is good enough.
    const bool gps_ok = ((gps_raw_int.fix_type >= 3) && (gps_raw_int.satellites_visible >= 8));

    set_health_global_position(gps_ok);
    // Local is not different from global for now until things like flow are in place.
    set_health_local_position(gps_ok);

    if (_gps_info_subscription) {
        auto callback = _gps_info_subscription;
        auto arg = gps_info();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    _parent->refresh_timeout_handler(_gps_raw_timeout_cookie);
}

void TelemetryImpl::process_ground_truth(const mavlink_message_t& message)
{
    mavlink_hil_state_quaternion_t hil_state_quaternion;
    mavlink_msg_hil_state_quaternion_decode(&message, &hil_state_quaternion);

    set_ground_truth(Telemetry::GroundTruth({hil_state_quaternion.lat * 1e-7,
                                             hil_state_quaternion.lon * 1e-7,
                                             hil_state_quaternion.alt * 1e-3f}));

    if (_ground_truth_subscription) {
        auto callback = _ground_truth_subscription;
        auto arg = ground_truth();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_extended_sys_state(const mavlink_message_t& message)
{
    mavlink_extended_sys_state_t extended_sys_state;
    mavlink_msg_extended_sys_state_decode(&message, &extended_sys_state);

    {
        Telemetry::LandedState landed_state = to_landed_state(extended_sys_state);
        set_landed_state(landed_state);
    }

    if (_landed_state_subscription) {
        auto callback = _landed_state_subscription;
        auto arg = landed_state();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    if (extended_sys_state.landed_state == MAV_LANDED_STATE_IN_AIR ||
        extended_sys_state.landed_state == MAV_LANDED_STATE_TAKEOFF ||
        extended_sys_state.landed_state == MAV_LANDED_STATE_LANDING) {
        set_in_air(true);
    } else if (extended_sys_state.landed_state == MAV_LANDED_STATE_ON_GROUND) {
        set_in_air(false);
    }
    // If landed_state is undefined, we use what we have received last.

    if (_in_air_subscription) {
        auto callback = _in_air_subscription;
        auto arg = in_air();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}
void TelemetryImpl::process_fixedwing_metrics(const mavlink_message_t& message)
{
    mavlink_vfr_hud_t vfr_hud;
    mavlink_msg_vfr_hud_decode(&message, &vfr_hud);

    set_fixedwing_metrics(
        Telemetry::FixedwingMetrics({vfr_hud.airspeed, vfr_hud.throttle * 1e-2f, vfr_hud.climb}));

    if (_fixedwing_metrics_subscription) {
        auto callback = _fixedwing_metrics_subscription;
        auto arg = fixedwing_metrics();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_sys_status(const mavlink_message_t& message)
{
    mavlink_sys_status_t sys_status;
    mavlink_msg_sys_status_decode(&message, &sys_status);
    set_battery(Telemetry::Battery(
        {sys_status.voltage_battery * 1e-3f,
         // FIXME: it is strange calling it percent when the range goes from 0 to 1.
         sys_status.battery_remaining * 1e-2f}));

    if (_battery_subscription) {
        auto callback = _battery_subscription;
        auto arg = battery();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_heartbeat(const mavlink_message_t& message)
{
    if (message.compid != MAV_COMP_ID_AUTOPILOT1) {
        return;
    }

    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&message, &heartbeat);

    set_armed(((heartbeat.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) ? true : false));

    if (_armed_subscription) {
        auto callback = _armed_subscription;
        auto arg = armed();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    if (_flight_mode_subscription) {
        auto callback = _flight_mode_subscription;
        // The flight mode is already parsed in SystemImpl, so we can take it
        // from there.  This assumes that SystemImpl gets called first because
        // it's earlier in the callback list.
        auto arg = telemetry_flight_mode_from_flight_mode(_parent->get_flight_mode());
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    if (_health_subscription) {
        auto callback = _health_subscription;
        auto arg = health();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
    if (_health_all_ok_subscription) {
        auto callback = _health_all_ok_subscription;
        auto arg = health_all_ok();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_statustext(const mavlink_message_t& message)
{
    mavlink_statustext_t statustext;
    mavlink_msg_statustext_decode(&message, &statustext);

    Telemetry::StatusType type;

    switch (statustext.severity) {
        case MAV_SEVERITY_WARNING:
            type = Telemetry::StatusType::Warning;
            break;
        case MAV_SEVERITY_CRITICAL:
            type = Telemetry::StatusType::Critical;
            break;
        case MAV_SEVERITY_INFO:
            type = Telemetry::StatusType::Info;
            break;
        default:
            LogWarn() << "Unknown StatusText severity";
            type = Telemetry::StatusType::Info;
            break;
    }

    // statustext.text is not null terminated, therefore we copy it first to
    // an array big enough that is zeroed.
    char text_with_null[sizeof(statustext.text) + 1]{};
    memcpy(text_with_null, statustext.text, sizeof(statustext.text));

    const std::string text = text_with_null;

    set_status_text({type, text});

    if (_status_text_subscription) {
        _status_text_subscription(status_text());
    }
}

void TelemetryImpl::process_rc_channels(const mavlink_message_t& message)
{
    mavlink_rc_channels_t rc_channels;
    mavlink_msg_rc_channels_decode(&message, &rc_channels);

    bool rc_ok = (rc_channels.chancount > 0);
    set_rc_status(rc_ok, rc_channels.rssi);

    if (_rc_status_subscription) {
        auto callback = _rc_status_subscription;
        auto arg = rc_status();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    _parent->refresh_timeout_handler(_rc_channels_timeout_cookie);
}

void TelemetryImpl::process_unix_epoch_time(const mavlink_message_t& message)
{
    mavlink_utm_global_position_t utm_global_position;
    mavlink_msg_utm_global_position_decode(&message, &utm_global_position);

    set_unix_epoch_time_us(utm_global_position.time);

    if (_unix_epoch_time_subscription) {
        auto callback = _unix_epoch_time_subscription;
        auto arg = unix_epoch_time();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }

    _parent->refresh_timeout_handler(_unix_epoch_timeout_cookie);
}

void TelemetryImpl::process_actuator_control_target(const mavlink_message_t& message)
{
    mavlink_set_actuator_control_target_t target;
    mavlink_msg_set_actuator_control_target_decode(&message, &target);

    uint32_t group = target.group_mlx;
    std::vector<float> controls;

    const unsigned control_size = sizeof(target.controls) / sizeof(target.controls[0]);
    // Can't use std::copy because target is packed.
    for (std::size_t i = 0; i < control_size; ++i) {
        controls[i] = target.controls[i];
    }

    set_actuator_control_target(group, controls);

    if (_actuator_control_target_subscription) {
        auto callback = _actuator_control_target_subscription;
        auto arg = actuator_control_target();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_actuator_output_status(const mavlink_message_t& message)
{
    mavlink_actuator_output_status_t status;
    mavlink_msg_actuator_output_status_decode(&message, &status);

    uint32_t active = status.active;
    std::vector<float> actuators;

    const unsigned actuators_size = sizeof(status.actuator) / sizeof(status.actuator[0]);
    // Can't use std::copy because status is packed.
    for (std::size_t i = 0; i < actuators_size; ++i) {
        actuators[i] = status.actuator[i];
    }

    set_actuator_output_status(active, actuators);

    if (_actuator_output_status_subscription) {
        auto callback = _actuator_output_status_subscription;
        auto arg = actuator_output_status();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

void TelemetryImpl::process_odometry(const mavlink_message_t& message)
{
    mavlink_odometry_t odometry_msg;
    mavlink_msg_odometry_decode(&message, &odometry_msg);

    Telemetry::Odometry odometry_struct{};

    odometry_struct.time_usec = odometry_msg.time_usec;
    odometry_struct.frame_id = static_cast<Telemetry::MavFrame>(odometry_msg.frame_id);
    odometry_struct.child_frame_id =
        static_cast<Telemetry::MavFrame>(odometry_msg.child_frame_id);

    odometry_struct.position_body.x_m = odometry_msg.x;
    odometry_struct.position_body.y_m = odometry_msg.y;
    odometry_struct.position_body.z_m = odometry_msg.z;

    odometry_struct.q.w = odometry_msg.q[0];
    odometry_struct.q.x = odometry_msg.q[1];
    odometry_struct.q.y = odometry_msg.q[2];
    odometry_struct.q.z = odometry_msg.q[3];

    odometry_struct.velocity_body.x_m_s = odometry_msg.vx;
    odometry_struct.velocity_body.y_m_s = odometry_msg.vy;
    odometry_struct.velocity_body.z_m_s = odometry_msg.vz;

    odometry_struct.angular_velocity_body.roll_rad_s = odometry_msg.rollspeed;
    odometry_struct.angular_velocity_body.pitch_rad_s = odometry_msg.pitchspeed;
    odometry_struct.angular_velocity_body.yaw_rad_s = odometry_msg.yawspeed;

    assert(
        odometry_struct.pose_covariance.covariance_matrix.size() ==
            sizeof(odometry_msg.pose_covariance) / sizeof(odometry_msg.pose_covariance[0]));
    // Can't use std::copy because odometry_msg is packed.
    for (std::size_t i = 0; i < odometry_struct.pose_covariance.covariance_matrix.size(); ++i) {
        odometry_struct.pose_covariance.covariance_matrix[i] = odometry_msg.pose_covariance[i];
    }

    assert(
        odometry_struct.velocity_covariance.covariance_matrix.size() ==
            sizeof(odometry_msg.velocity_covariance) / sizeof(odometry_msg.velocity_covariance[0]));
    // Can't use std::copy because odometry_msg is packed.
    for (std::size_t i = 0; i < odometry_struct.velocity_covariance.covariance_matrix.size(); ++i) {
        odometry_struct.velocity_covariance.covariance_matrix[i] = odometry_msg.velocity_covariance[i];
    }

    set_odometry(odometry_struct);

    if (_odometry_subscription) {
        auto callback = _odometry_subscription;
        auto arg = odometry();
        _parent->call_user_callback([callback, arg]() { callback(arg); });
    }
}

Telemetry::LandedState
TelemetryImpl::to_landed_state(mavlink_extended_sys_state_t extended_sys_state)
{
    switch (extended_sys_state.landed_state) {
        case MAV_LANDED_STATE_IN_AIR:
            return Telemetry::LandedState::LandedStateInAir;
        case MAV_LANDED_STATE_TAKEOFF:
            return Telemetry::LandedState::LandedStateTakingOff;
        case MAV_LANDED_STATE_LANDING:
            return Telemetry::LandedState::LandedStateLanding;
        case MAV_LANDED_STATE_ON_GROUND:
            return Telemetry::LandedState::LandedStateOnGround;
        default:
            return Telemetry::LandedState::LandedStateUnknown;
    }
}

Telemetry::FlightMode
TelemetryImpl::telemetry_flight_mode_from_flight_mode(SystemImpl::FlightMode flight_mode)
{
    switch (flight_mode) {
        case SystemImpl::FlightMode::READY:
            return Telemetry::FlightMode::Ready;
        case SystemImpl::FlightMode::TAKEOFF:
            return Telemetry::FlightMode::Takeoff;
        case SystemImpl::FlightMode::HOLD:
            return Telemetry::FlightMode::Hold;
        case SystemImpl::FlightMode::MISSION:
            return Telemetry::FlightMode::Mission;
        case SystemImpl::FlightMode::RETURN_TO_LAUNCH:
            return Telemetry::FlightMode::ReturnToLaunch;
        case SystemImpl::FlightMode::LAND:
            return Telemetry::FlightMode::Land;
        case SystemImpl::FlightMode::OFFBOARD:
            return Telemetry::FlightMode::Offboard;
        case SystemImpl::FlightMode::FOLLOW_ME:
            return Telemetry::FlightMode::FollowMe;
        case SystemImpl::FlightMode::MANUAL:
            return Telemetry::FlightMode::Manual;
        case SystemImpl::FlightMode::POSCTL:
            return Telemetry::FlightMode::Posctl;
        case SystemImpl::FlightMode::ALTCTL:
            return Telemetry::FlightMode::Altctl;
        case SystemImpl::FlightMode::RATTITUDE:
            return Telemetry::FlightMode::Rattitude;
        case SystemImpl::FlightMode::ACRO:
            return Telemetry::FlightMode::Acro;
        case SystemImpl::FlightMode::STABILIZED:
            return Telemetry::FlightMode::Stabilized;
        default:
            return Telemetry::FlightMode::Unknown;
    }
}

void TelemetryImpl::receive_param_cal_gyro(MAVLinkParameters::Result result, int value)
{
    if (result != MAVLinkParameters::Result::SUCCESS) {
        LogErr() << "Error: Param for gyro cal failed.";
        return;
    }

    bool ok = (value != 0);
    set_health_gyrometer_calibration(ok);
}

void TelemetryImpl::receive_param_cal_accel(MAVLinkParameters::Result result, int value)
{
    if (result != MAVLinkParameters::Result::SUCCESS) {
        LogErr() << "Error: Param for accel cal failed.";
        return;
    }

    bool ok = (value != 0);
    set_health_accelerometer_calibration(ok);
}

void TelemetryImpl::receive_param_cal_mag(MAVLinkParameters::Result result, int value)
{
    if (result != MAVLinkParameters::Result::SUCCESS) {
        LogErr() << "Error: Param for mag cal failed.";
        return;
    }

    bool ok = (value != 0);
    set_health_magnetometer_calibration(ok);
}

#ifdef LEVEL_CALIBRATION
void TelemetryImpl::receive_param_cal_level(MAVLinkParameters::Result result, float value)
{
    if (result != MAVLinkParameters::Result::SUCCESS) {
        LogErr() << "Error: Param for level cal failed.";
        return;
    }

    bool ok = (value != 0);
    set_health_level_calibration(ok);
}
#endif

void TelemetryImpl::receive_param_hitl(MAVLinkParameters::Result result, int value)
{
    if (result != MAVLinkParameters::Result::SUCCESS) {
        LogErr() << "Error: Param to determine hitl failed.";
        return;
    }

    _hitl_enabled = (value == 1);

    // assume sensor calibration ok in hitl
    if (_hitl_enabled) {
        set_health_accelerometer_calibration(_hitl_enabled);
        set_health_gyrometer_calibration(_hitl_enabled);
        set_health_magnetometer_calibration(_hitl_enabled);
    }
#ifdef LEVEL_CALIBRATION
    set_health_level_calibration(ok);
#endif
}

void TelemetryImpl::receive_rc_channels_timeout()
{
    const bool rc_ok = false;
    set_rc_status(rc_ok, 0.0f);
}

void TelemetryImpl::receive_gps_raw_timeout()
{
    const bool position_ok = false;
    set_health_local_position(position_ok);
    set_health_global_position(position_ok);
}

void TelemetryImpl::receive_unix_epoch_timeout()
{
    const uint64_t unix_epoch = 0;
    set_unix_epoch_time_us(unix_epoch);
}

Telemetry::PositionVelocityNed TelemetryImpl::position_velocity_ned() const
{
    std::lock_guard<std::mutex> lock(_position_velocity_ned_mutex);
    return _position_velocity_ned;
}

void TelemetryImpl::set_position_velocity_ned(Telemetry::PositionVelocityNed position_velocity_ned)
{
    std::lock_guard<std::mutex> lock(_position_velocity_ned_mutex);
    _position_velocity_ned = position_velocity_ned;
}

Telemetry::Position TelemetryImpl::position() const
{
    std::lock_guard<std::mutex> lock(_position_mutex);
    return _position;
}

void TelemetryImpl::set_position(Telemetry::Position position)
{
    std::lock_guard<std::mutex> lock(_position_mutex);
    _position = position;
}

Telemetry::Position TelemetryImpl::home() const
{
    std::lock_guard<std::mutex> lock(_home_position_mutex);
    return _home_position;
}

void TelemetryImpl::set_home_position(Telemetry::Position home_position)
{
    std::lock_guard<std::mutex> lock(_home_position_mutex);
    _home_position = home_position;
}

bool TelemetryImpl::armed() const
{
    return _armed;
}

bool TelemetryImpl::in_air() const
{
    return _in_air;
}

void TelemetryImpl::set_in_air(bool in_air_new)
{
    _in_air = in_air_new;
}

void TelemetryImpl::set_status_text(Telemetry::StatusText status_text)
{
    std::lock_guard<std::mutex> lock(_status_text_mutex);
    _status_text = status_text;
}

Telemetry::StatusText TelemetryImpl::status_text() const
{
    std::lock_guard<std::mutex> lock(_status_text_mutex);
    return _status_text;
}

void TelemetryImpl::set_armed(bool armed_new)
{
    _armed = armed_new;
}

Telemetry::Quaternion TelemetryImpl::attitude_quaternion() const
{
    std::lock_guard<std::mutex> lock(_attitude_quaternion_mutex);
    return _attitude_quaternion;
}

Telemetry::AngularVelocityBody TelemetryImpl::attitude_angular_velocity_body() const
{
    std::lock_guard<std::mutex> lock(_attitude_angular_velocity_body_mutex);
    return _attitude_angular_velocity_body;
}

Telemetry::GroundTruth TelemetryImpl::ground_truth() const
{
    std::lock_guard<std::mutex> lock(_ground_truth_mutex);
    return _ground_truth;
}

Telemetry::FixedwingMetrics TelemetryImpl::fixedwing_metrics() const
{
    std::lock_guard<std::mutex> lock(_fixedwing_metrics_mutex);
    return _fixedwing_metrics;
}

Telemetry::EulerAngle TelemetryImpl::attitude_euler() const
{
    std::lock_guard<std::mutex> lock(_attitude_quaternion_mutex);
    Telemetry::EulerAngle euler = to_euler_angle_from_quaternion(_attitude_quaternion);

    return euler;
}

void TelemetryImpl::set_attitude_quaternion(Telemetry::Quaternion quaternion)
{
    std::lock_guard<std::mutex> lock(_attitude_quaternion_mutex);
    _attitude_quaternion = quaternion;
}

void TelemetryImpl::set_attitude_angular_velocity_body(
    Telemetry::AngularVelocityBody angular_velocity_body)
{
    std::lock_guard<std::mutex> lock(_attitude_quaternion_mutex);
    _attitude_angular_velocity_body = angular_velocity_body;
}

void TelemetryImpl::set_ground_truth(Telemetry::GroundTruth ground_truth)
{
    std::lock_guard<std::mutex> lock(_ground_truth_mutex);
    _ground_truth = ground_truth;
}

void TelemetryImpl::set_fixedwing_metrics(Telemetry::FixedwingMetrics fixedwing_metrics)
{
    std::lock_guard<std::mutex> lock(_fixedwing_metrics_mutex);
    _fixedwing_metrics = fixedwing_metrics;
}

Telemetry::Quaternion TelemetryImpl::camera_attitude_quaternion() const
{
    std::lock_guard<std::mutex> lock(_camera_attitude_euler_angle_mutex);
    Telemetry::Quaternion quaternion = to_quaternion_from_euler_angle(_camera_attitude_euler_angle);

    return quaternion;
}

Telemetry::EulerAngle TelemetryImpl::camera_attitude_euler() const
{
    std::lock_guard<std::mutex> lock(_camera_attitude_euler_angle_mutex);

    return _camera_attitude_euler_angle;
}

void TelemetryImpl::set_camera_attitude_euler_angle(Telemetry::EulerAngle euler_angle)
{
    std::lock_guard<std::mutex> lock(_camera_attitude_euler_angle_mutex);
    _camera_attitude_euler_angle = euler_angle;
}

Telemetry::SpeedNed TelemetryImpl::ground_speed_ned() const
{
    std::lock_guard<std::mutex> lock(_ground_speed_ned_mutex);
    return _ground_speed_ned;
}

void TelemetryImpl::set_ground_speed_ned(Telemetry::SpeedNed ground_speed_ned)
{
    std::lock_guard<std::mutex> lock(_ground_speed_ned_mutex);
    _ground_speed_ned = ground_speed_ned;
}

Telemetry::Imu TelemetryImpl::imu() const
{
    std::lock_guard<std::mutex> lock(_imu_reading_ned_mutex);
    return _imu_reading_ned;
}

void TelemetryImpl::set_imu_reading_ned(Telemetry::Imu imu_reading_ned)
{
    std::lock_guard<std::mutex> lock(_imu_reading_ned_mutex);
    _imu_reading_ned = imu_reading_ned;
}

Telemetry::GpsInfo TelemetryImpl::gps_info() const
{
    std::lock_guard<std::mutex> lock(_gps_info_mutex);
    return _gps_info;
}

void TelemetryImpl::set_gps_info(Telemetry::GpsInfo gps_info)
{
    std::lock_guard<std::mutex> lock(_gps_info_mutex);
    _gps_info = gps_info;
}

Telemetry::Battery TelemetryImpl::battery() const
{
    std::lock_guard<std::mutex> lock(_battery_mutex);
    return _battery;
}

void TelemetryImpl::set_battery(Telemetry::Battery battery)
{
    std::lock_guard<std::mutex> lock(_battery_mutex);
    _battery = battery;
}

Telemetry::FlightMode TelemetryImpl::flight_mode() const
{
    return telemetry_flight_mode_from_flight_mode(_parent->get_flight_mode());
}

Telemetry::Health TelemetryImpl::health() const
{
    std::lock_guard<std::mutex> lock(_health_mutex);
    return _health;
}

bool TelemetryImpl::health_all_ok() const
{
    std::lock_guard<std::mutex> lock(_health_mutex);
    if (_health.is_gyrometer_calibration_ok && _health.is_accelerometer_calibration_ok &&
        _health.is_magnetometer_calibration_ok && _health.is_level_calibration_ok &&
        _health.is_local_position_ok && _health.is_global_position_ok && _health.is_home_position_ok) {
        return true;
    } else {
        return false;
    }
}

Telemetry::RcStatus TelemetryImpl::rc_status() const
{
    std::lock_guard<std::mutex> lock(_rc_status_mutex);
    return _rc_status;
}

uint64_t TelemetryImpl::unix_epoch_time() const
{
    std::lock_guard<std::mutex> lock(_unix_epoch_time_mutex);
    return _unix_epoch_time_us;
}

Telemetry::ActuatorControlTarget TelemetryImpl::actuator_control_target() const
{
    std::lock_guard<std::mutex> lock(_actuator_control_target_mutex);
    return _actuator_control_target;
}

Telemetry::ActuatorOutputStatus TelemetryImpl::actuator_output_status() const
{
    std::lock_guard<std::mutex> lock(_actuator_output_status_mutex);
    return _actuator_output_status;
}

Telemetry::Odometry TelemetryImpl::odometry() const
{
    std::lock_guard<std::mutex> lock(_odometry_mutex);
    return _odometry;
}

void TelemetryImpl::set_health_local_position(bool ok)
{
    std::lock_guard<std::mutex> lock(_health_mutex);
    _health.is_local_position_ok = ok;
}

void TelemetryImpl::set_health_global_position(bool ok)
{
    std::lock_guard<std::mutex> lock(_health_mutex);
    _health.is_global_position_ok = ok;
}

void TelemetryImpl::set_health_home_position(bool ok)
{
    std::lock_guard<std::mutex> lock(_health_mutex);
    _health.is_home_position_ok = ok;
}

void TelemetryImpl::set_health_gyrometer_calibration(bool ok)
{
    std::lock_guard<std::mutex> lock(_health_mutex);
    _health.is_gyrometer_calibration_ok = (ok || _hitl_enabled);
}

void TelemetryImpl::set_health_accelerometer_calibration(bool ok)
{
    std::lock_guard<std::mutex> lock(_health_mutex);
    _health.is_accelerometer_calibration_ok = (ok || _hitl_enabled);
}

void TelemetryImpl::set_health_magnetometer_calibration(bool ok)
{
    std::lock_guard<std::mutex> lock(_health_mutex);
    _health.is_magnetometer_calibration_ok = (ok || _hitl_enabled);
}

void TelemetryImpl::set_health_level_calibration(bool ok)
{
    std::lock_guard<std::mutex> lock(_health_mutex);
    _health.is_level_calibration_ok = (ok || _hitl_enabled);
}

Telemetry::LandedState TelemetryImpl::landed_state() const
{
    std::lock_guard<std::mutex> lock(_landed_state_mutex);
    return _landed_state;
}

void TelemetryImpl::set_landed_state(Telemetry::LandedState landed_state)
{
    std::lock_guard<std::mutex> lock(_landed_state_mutex);
    _landed_state = landed_state;
}

void TelemetryImpl::set_rc_status(bool available, float signal_strength_percent)
{
    std::lock_guard<std::mutex> lock(_rc_status_mutex);

    if (available) {
        _rc_status.was_available_once = true;
        _rc_status.signal_strength_percent = signal_strength_percent;
    } else {
        _rc_status.signal_strength_percent = 0.0f;
    }

    _rc_status.is_available = available;
}

void TelemetryImpl::set_unix_epoch_time_us(uint64_t time_us)
{
    std::lock_guard<std::mutex> lock(_unix_epoch_time_mutex);
    _unix_epoch_time_us = time_us;
}

void TelemetryImpl::set_actuator_control_target(uint8_t group, const std::vector<float>& controls)
{
    std::lock_guard<std::mutex> lock(_actuator_control_target_mutex);
    _actuator_control_target.group = group;
    _actuator_control_target.controls = controls;
}

void TelemetryImpl::set_actuator_output_status(
    uint32_t active, const std::vector<float>& actuators)
{
    std::lock_guard<std::mutex> lock(_actuator_output_status_mutex);
    _actuator_output_status.active = active;
    _actuator_output_status.actuator = actuators;
}

void TelemetryImpl::set_odometry(Telemetry::Odometry& odometry)
{
    std::lock_guard<std::mutex> lock(_actuator_output_status_mutex);
    _odometry = odometry;
}

void TelemetryImpl::position_velocity_ned_async(
    Telemetry::position_velocity_ned_callback_t& callback)
{
    _position_velocity_ned_subscription = callback;
}

void TelemetryImpl::position_async(Telemetry::position_callback_t& callback)
{
    _position_subscription = callback;
}

void TelemetryImpl::home_async(Telemetry::position_callback_t& callback)
{
    _home_position_subscription = callback;
}

void TelemetryImpl::in_air_async(Telemetry::in_air_callback_t& callback)
{
    _in_air_subscription = callback;
}

void TelemetryImpl::status_text_async(Telemetry::status_text_callback_t& callback)
{
    _status_text_subscription = callback;
}

void TelemetryImpl::armed_async(Telemetry::armed_callback_t& callback)
{
    _armed_subscription = callback;
}

void TelemetryImpl::attitude_quaternion_async(Telemetry::attitude_quaternion_callback_t& callback)
{
    _attitude_quaternion_angle_subscription = callback;
}

void TelemetryImpl::attitude_euler_async(Telemetry::attitude_euler_callback_t& callback)
{
    _attitude_euler_angle_subscription = callback;
}

void TelemetryImpl::attitude_angular_velocity_body_async(
    Telemetry::attitude_angular_velocity_body_callback_t& callback)
{
    _attitude_angular_velocity_body_subscription = callback;
}

void TelemetryImpl::fixedwing_metrics_async(Telemetry::fixedwing_metrics_callback_t& callback)
{
    _fixedwing_metrics_subscription = callback;
}

void TelemetryImpl::ground_truth_async(Telemetry::ground_truth_callback_t& callback)
{
    _ground_truth_subscription = callback;
}

void TelemetryImpl::camera_attitude_quaternion_async(
    Telemetry::attitude_quaternion_callback_t& callback)
{
    _camera_attitude_quaternion_subscription = callback;
}

void TelemetryImpl::camera_attitude_euler_async(
    Telemetry::attitude_euler_callback_t& callback)
{
    _camera_attitude_euler_angle_subscription = callback;
}

void TelemetryImpl::ground_speed_ned_async(Telemetry::ground_speed_ned_callback_t& callback)
{
    _ground_speed_ned_subscription = callback;
}

void TelemetryImpl::imu_async(Telemetry::imu_callback_t& callback)
{
    _imu_reading_ned_subscription = callback;
}

void TelemetryImpl::gps_info_async(Telemetry::gps_info_callback_t& callback)
{
    _gps_info_subscription = callback;
}

void TelemetryImpl::battery_async(Telemetry::battery_callback_t& callback)
{
    _battery_subscription = callback;
}

void TelemetryImpl::flight_mode_async(Telemetry::flight_mode_callback_t& callback)
{
    _flight_mode_subscription = callback;
}

void TelemetryImpl::health_async(Telemetry::health_callback_t& callback)
{
    _health_subscription = callback;
}

void TelemetryImpl::health_all_ok_async(Telemetry::health_all_ok_callback_t& callback)
{
    _health_all_ok_subscription = callback;
}

void TelemetryImpl::landed_state_async(Telemetry::landed_state_callback_t& callback)
{
    _landed_state_subscription = callback;
}

void TelemetryImpl::rc_status_async(Telemetry::rc_status_callback_t& callback)
{
    _rc_status_subscription = callback;
}

void TelemetryImpl::unix_epoch_time_async(Telemetry::unix_epoch_time_callback_t& callback)
{
    _unix_epoch_time_subscription = callback;
}

void TelemetryImpl::actuator_control_target_async(
    Telemetry::actuator_control_target_callback_t& callback)
{
    _actuator_control_target_subscription = callback;
}

void TelemetryImpl::actuator_output_status_async(
    Telemetry::actuator_output_status_callback_t& callback)
{
    _actuator_output_status_subscription = callback;
}

void TelemetryImpl::odometry_async(Telemetry::odometry_callback_t& callback)
{
    _odometry_subscription = callback;
}

void TelemetryImpl::process_parameter_update(const std::string& name)
{
    if (name.compare("CAL_GYRO0_ID") == 0) {
        _parent->get_param_int_async(
            std::string("CAL_GYRO0_ID"),
            std::bind(
                &TelemetryImpl::receive_param_cal_gyro,
                this,
                std::placeholders::_1,
                std::placeholders::_2),
            this);

    } else if (name.compare("CAL_ACC0_ID") == 0) {
        _parent->get_param_int_async(
            std::string("CAL_ACC0_ID"),
            std::bind(
                &TelemetryImpl::receive_param_cal_accel,
                this,
                std::placeholders::_1,
                std::placeholders::_2),
            this);

    } else if (name.compare("CAL_MAG0_ID") == 0) {
        _parent->get_param_int_async(
            std::string("CAL_MAG0_ID"),
            std::bind(
                &TelemetryImpl::receive_param_cal_mag,
                this,
                std::placeholders::_1,
                std::placeholders::_2),
            this);

#ifdef LEVEL_CALIBRATION
    } else if (name.compare("SENS_BOARD_X_OFF") == 0) {
        _parent->get_param_float_async(
            std::string("SENS_BOARD_X_OFF"),
            std::bind(
                &TelemetryImpl::receive_param_cal_level,
                this,
                std::placeholders::_1,
                std::placeholders::_2),
            this);
#endif
    } else if (name.compare("SYS_HITL") == 0) {
        _parent->get_param_int_async(
            std::string("SYS_HITL"),
            std::bind(
                &TelemetryImpl::receive_param_hitl,
                this,
                std::placeholders::_1,
                std::placeholders::_2),
            this);
    }
}

} // namespace mavsdk
