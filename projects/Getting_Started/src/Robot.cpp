#include "WPILib.h"
#include "Commands/ExtendRightWiper.h"
#include "Commands/RetractRightWiper.h"
#include "Utilities.h"
#include "RobotDefinitions.h"
#include "ElevatorController.h"
#include "DerivedCameraServer.h"
#include <math.h>


class Robot: public IterativeRobot {
	Gyro *rateGyro;
	RobotDrive myRobot; // robot drive system
	Jaguar *elevatorMotorA;
	Jaguar *elevatorMotorB;
	Jaguar *swingArmMotor;
	Joystick rightStick;
	Joystick leftStick;
	Joystick controlBox;
	DoubleSolenoid *dsLeft;
	DoubleSolenoid *dsRight;
	DoubleSolenoid *dsGrappler;
//Camera Servo
	Servo *camYServo;
	Servo *camXServo;


	LiveWindow *lw;
	USBCamera *camera;
	CameraServer *cameraServer;

	Relay *leftIntakeWheel;
	Relay *rightIntakeWheel;

	JoystickButton *button1;
	JoystickButton *button2;
	AnalogPotentiometer *elevatorPot;
	AnalogInput *elevatorVertPotInput;
	AnalogInput *elevatorHorizPotInput;
	ElevatorController *elevatorController;

	AnalogPotentiometer *vertPot;

//	PIDController *elevatorPidController_0;
//	PIDController *elevatorPidController_1;

	int autoLoopCounter;
	int teleLoopCounter;
	float prevAngle;
	float prevPot;
	float prevLeftEnc;
	float prevRightEnc;
	float autoDistCounter;
	float autoMaxDistance;
	float autoGyroAngle;
	int autoState;
	bool b[7];
	int wiperState = 0;
	bool grappling = true;

	Compressor *compressor;
	Encoder *encLeft;
	Encoder *encRight;

public:
	Robot() :
		myRobot(FRONT_LEFT_MOTOR_CHANNEL,
				REAR_LEFT_MOTOR_CHANNEL,
				FRONT_RIGHT_MOTOR_CHANNEL,
				REAR_RIGHT_MOTOR_CHANNEL),	// these must be initialized in the same order
											// as they are declared above.
		rightStick(LEFT_JOYSTICK_USB_PORT),
		leftStick(RIGHT_JOYSTICK_USB_PORT),
		controlBox(CONTROL_BOX_USB_PORT),
		lw(NULL),
		autoLoopCounter(0)
		{

		myRobot.SetExpiration(0.1);
		//myRobot.SetInvertedMotor(MotorType::)

		// This code enables the USB Microsoft Camera display.
		// You must pick "USB Camera HW" on the Driverstation Dashboard
		// the name of the camera "cam1" can be found in the RoboRio web dashboard
//		CameraServer::GetInstance()->SetQuality(90);
//		CameraServer::GetInstance()->SetSize(2);
//		CameraServer::GetInstance()->StartAutomaticCapture("cam1");
//		CameraServer::GetInstance()->m_camera->SetExposureAuto();
		DerivedCameraServer *cameraServer = DerivedCameraServer::GetInstance();
		cameraServer->SetQuality(90);
		cameraServer->SetSize(2);
		cameraServer->StartAutomaticCapture("cam1");
		cameraServer->setExposureAuto();
		cameraServer->setWhiteBalanceAuto();

		compressor = new Compressor();
		rateGyro   = new Gyro(GYRO_RATE_INPUT_CHANNEL);
		dsLeft     = new DoubleSolenoid(LEFT_WIPER_SOLENOID_FWD_CHANNEL, LEFT_WIPER_SOLENOID_REV_CHANNEL);
		dsRight    = new DoubleSolenoid(RIGHT_WIPER_SOLENOID_FWD_CHANNEL, RIGHT_WIPER_SOLENOID_REV_CHANNEL);
		dsGrappler = new DoubleSolenoid(GRAPPLER_SOLENOID_FWD_CHANNEL, GRAPPLER_SOLENOID_REV_CHANNEL);

		encLeft = new Encoder(LEFT_WHEEL_ENCODER_CHANNEL_A, LEFT_WHEEL_ENCODER_CHANNEL_B,
								false, Encoder::EncodingType::k4X);
		encRight = new Encoder(RIGHT_WHEEL_ENCODER_CHANNEL_A, RIGHT_WHEEL_ENCODER_CHANNEL_B,
								true, Encoder::EncodingType::k4X);
		encLeft->SetDistancePerPulse(WHEEL_DIAMETER * M_PI / PULSES_PER_ROTATION); // 4" diameter wheel * PI / 360 pulses/rotation
		encRight->SetDistancePerPulse(WHEEL_DIAMETER * M_PI / PULSES_PER_ROTATION);
		leftIntakeWheel = new Relay(LEFT_INTAKE_WHEEL_CHANNEL);
		rightIntakeWheel = new Relay(RIGHT_INTAKE_WHEEL_CHANNEL);

		elevatorVertPotInput = new AnalogInput(ELEVATOR_VERT_INPUT_CHANNEL);
		elevatorHorizPotInput = new AnalogInput(ELEVATOR_HORIZ_INPUT_CHANNEL);
		elevatorMotorA = new Jaguar(ELEVATOR_MOTOR_CHANNEL_A);
		elevatorMotorB = new Jaguar(ELEVATOR_MOTOR_CHANNEL_B);
		swingArmMotor  = new Jaguar(SWING_ARM_MOTOR_CHANNEL);

		button1 = new JoystickButton(&rightStick, 1);
		button2 = new JoystickButton(&rightStick, 2);

		camYServo = new Servo(9);
		camXServo = new Servo(8);


		//button1->ToggleWhenPressed(new ExtendRightWiper(doubleSolenoid));
		//button2->ToggleWhenPressed(new RetractRightWiper(doubleSolenoid));

		// fill in horizontal relay variable when available
//		elevatorController = new ElevatorController(elevatorVertPotInput, elevatorHorizPotInput,
//													elevatorMotorA, elevatorMotorB, (Relay*)NULL);
		// analog input min value: 1.352539
		// analog input max value: 4.964599
		// analog range = 3.61206
		// 0.992130
		vertPot = new AnalogPotentiometer(elevatorVertPotInput, 5.0, -1.372931);
//		elevatorPidController_0 = new PIDController(0.5, 0.0, 0.0, vertPot, elevatorMotorA);
//		elevatorPidController_1 = new PIDController(0.5, 0.0, 0.0, vertPot, elevatorMotorB);

	}

private:
	void RobotInit() {
		lw = LiveWindow::GetInstance();
		printf("Team 812 - It's alive! 2015-02-12\n");
	}
	void DisabledInit() {
		printf("Team 812 - DisabledInit\n");
	}
	void DisabledPeriodic() {
		//printf("Team 812 - DisabledPeriodic\n");
	}

	void AutonomousInit() {
		autoLoopCounter = 0;
		autoDistCounter = 0;
		autoMaxDistance = 12.0 * 4.0 * 3.14159;  // 12 rotations * 4" diameter wheel * PI
		autoGyroAngle = 0;
		autoState = 0;
		encRight->Reset();
		encLeft->Reset();
		rateGyro->Reset();

		for (int i = 1; i <= 7; i++) {
			b[i] = controlBox.GetRawButton(i);
			printf("button[%d] = %s\n", i, b[i] ? "true":"false");
		}

		//printf("Hello 812!");

	}

	void AutonomousPeriodic() {

		double robotDriveCurve;
		if(autoLoopCounter++ < 500) {
			if(b[4]==0) {

				if( autoState == 0 ) {	// make sure the elevator is down
					elevatorMotorA->Set(-0.1);
					elevatorMotorB->Set(-0.1);
					Wait(0.5);
					elevatorMotorA->Set(0.0);
					elevatorMotorB->Set(0.0);
					autoState = 1;
				}
				if( autoState == 1) { // drive the robot into the box a bit
					myRobot.Drive(-0.15,0.0);
					Wait(0.5);
					autoState = 2;
				}
				if( autoState == 2) { // pick up the box
					elevatorMotorA->Set(0.5);
					elevatorMotorB->Set(0.5);
					Wait(0.2);
					myRobot.Drive(0.0, 0.0); // stop driving forward
					Wait(0.3);
					elevatorMotorA->Set(0.1);
					elevatorMotorB->Set(0.1);
					autoState = 3;
					encRight->Reset();
					encLeft->Reset();
			//		rateGyro->Reset();
				}
				if( autoState == 3) {
					autoDistCounter = encRight->GetDistance();
					autoGyroAngle = rateGyro->GetAngle();
					robotDriveCurve = PwrLimit(-autoGyroAngle * 1.2, -1.0, 1.0);

					if (-autoDistCounter <= BACKUP_INCHES && autoState == 3)
					{
						printf("Distance: %f, Turn direction: %f, Direction error: %f, Goal: %f\n",
								autoDistCounter, robotDriveCurve, autoGyroAngle, -BACKUP_INCHES);

						myRobot.Drive(BACKUP_SPEED, robotDriveCurve); // drive forwards half speed
						Wait(0.02);
					} else {
						myRobot.Drive(0.0, 0.0);
						autoState = 4;
					}
				}
//				if (autoState == 1) {
//					if (autoGyroAngle > -90.0 && autoState == 1) {
//						printf("Try turning left, autoGyroAngle = %f\n", autoGyroAngle);
//						myRobot.Drive(-0.2, -1.0);
//						Wait(0.01);
//					} else {
//						autoState = 2;
//						myRobot.Drive(0.0, 0.0); 	// stop robot
//						printf("Robot stopped\n");
//					}
//				}
			} else {
				printf("Autonomous mode is OFF\n");
				Wait(3.0);
			}
		}
		myRobot.Drive(0.0, 0.0);
	}

	void TeleopInit() {
		printf("Team 812 - Tally Ho! You're in control.\n");
		encRight->Reset();
		encLeft->Reset();
		rateGyro->Reset();
//		elevatorPidController_0->Enable();
//		elevatorPidController_0->SetSetpoint(1.0);
//		elevatorPidController_1->Enable();
//		elevatorPidController_1->SetSetpoint(1.0);
	}

	void TeleopPeriodic() {
		float currAngle;
		float currLeftEnc;
		float currRightEnc;
		float currPot;
		double elevatorPower, swingArmPower;

		myRobot.ArcadeDrive(PwrLimit(Linearize( rightStick.GetY()),-0.8, 0.8),
							PwrLimit(Linearize(-rightStick.GetX()),-0.65, 0.65)); // drive with arcade style (use right stick)

//		if(leftStick.GetRawButton(4)){
//			float setpoint = elevatorPidController_0->GetSetpoint();
//			setpoint -= 0.1;
//			if(setpoint < 0.0){
//				setpoint = 0.0;
//			}
//			elevatorPidController_0->SetSetpoint(setpoint);
//			elevatorPidController_1->SetSetpoint(setpoint);
//		}
//		if(leftStick.GetRawButton(5)){
//			float setpoint = elevatorPidController_0->GetSetpoint();
//			setpoint += 0.1;
//			if(setpoint > 4.0){
//				setpoint = 4.0;
//			}
//			elevatorPidController_0->SetSetpoint(setpoint);
//			elevatorPidController_1->SetSetpoint(setpoint);
//		}
		if(leftStick.GetRawButton(3)){
			//vertPot = new AnalogPotentiometer(elevatorVertPotInput
			printf("Analog input: %f, Potentiometer output: %f \n", elevatorVertPotInput->GetVoltage(), vertPot->Get());
		}

		currAngle = rateGyro->GetAngle();
		if (fabs(currAngle - prevAngle) > 0.10) {
			printf("gyro angle = %f, %f, %f\n", currAngle, prevAngle,
					fabs(currAngle - prevAngle));
			prevAngle = currAngle;
		}
		/* This Code causes the robot to crash due to timeouts as elevatorPot is not instantiated
		 * 2015-01-10 dano

		currPot = elevatorPot->Get();
		if (fabs(currPot - prevPot) > 0.01) {
			printf("pot reading: %f\n", currPot);
			prevPot = currPot;
		}
*/

		currLeftEnc = encLeft->GetDistance();
		currRightEnc = encRight->GetDistance();
		if (fabs(currLeftEnc - prevLeftEnc) + fabs(currRightEnc - prevRightEnc)
				> 0.01) {
			printf("Left Encoder = %f\n", currLeftEnc);
			printf("Right Encoder = %f\n", currRightEnc);
			prevLeftEnc = currLeftEnc;
			prevRightEnc = currRightEnc;
		}

		Scheduler::GetInstance()->Run();

		for (int i = 1; i <= 7; i++) {
			b[i] = controlBox.GetRawButton(i);
		}

		if (b[1] == 0 && b[2] == 1 && wiperState != 1) {
			printf("Tail off\n");
			wiperState = 1;
			// Pistons should be contracted. Wedge should be closed
			//Retract left piston.
			dsLeft->Set(DoubleSolenoid::kReverse);
			Wait(.2);
			dsLeft->Set(DoubleSolenoid::kOff);
			//Retract right piston
			dsRight->Set(DoubleSolenoid::kReverse);
			Wait(.2);
			dsRight->Set(DoubleSolenoid::kOff);

		} else if (b[1] == 0 && b[2] == 0 && wiperState != 2) {
			printf("Tail Rev\n");
			wiperState = 2;
			//Left side wedge should be open. Left piston extended and right piston contracted
			//Extend left piston
			dsLeft->Set(DoubleSolenoid::kForward);
			Wait(.2);
			dsLeft->Set(DoubleSolenoid::kOff);
			//Retract right piston
			dsRight->Set(DoubleSolenoid::kReverse);
			Wait(.2);
			dsRight->Set(DoubleSolenoid::kOff);
		} else if (b[1] == 1 && b[2] == 1 && wiperState != 3) {
			printf("Tail Fwd\n");
			wiperState = 3;
			//Right side wedge should be open. Right piston extended and left piston contracted.
			//Extend right piston
			dsRight->Set(DoubleSolenoid::kForward);
			Wait(.2);
			dsRight->Set(DoubleSolenoid::kOff);
			//Retract left piston
			dsLeft->Set(DoubleSolenoid::kReverse);
			Wait(.2);
			dsLeft->Set(DoubleSolenoid::kOff);
		} else if (b[1] && !b[2]) {
			printf("Uh oh Button problem!\n");
			wiperState = 0;
		}
		if (rightStick.GetRawButton(10)) {
			encLeft->Reset();
			encRight->Reset();
			rateGyro->Reset();
		}
		if (rightStick.GetRawButton(4)) {
			printf("Button 4 pressed - intake wheels forward\n");
			leftIntakeWheel->Set(Relay::kForward);
			rightIntakeWheel->Set(Relay::kForward);
		}
		if (rightStick.GetRawButton(5)) {
			printf("Button 5 pressed - intake wheels reverse\n");
			leftIntakeWheel->Set(Relay::kReverse);
			rightIntakeWheel->Set(Relay::kReverse);
		}
		if (rightStick.GetRawButton(6)) {
			printf("Button 6 pressed - intake wheels off\n");
			leftIntakeWheel->Set(Relay::kOff);
			rightIntakeWheel->Set(Relay::kOff);
		}

/* Manual elevator control
 * - The Linearize() function is a polynomial that scales the joystick output
 *   along a predefined curve thus dampening the power at low increments
 */
		elevatorPower = PwrLimit(Linearize(leftStick.GetY()),-0.2, 0.5); // Joystick Y position, low limit, high limit
		swingArmPower = PwrLimit(Linearize(leftStick.GetX()), -0.3, 0.3);
//		printf("Elevator power: %f, Swing arm power: %f\n", elevatorPower, swingArmPower);
//		printf("Elevator pot = %f\n", elevatorVertPotInput->GetVoltage());
		elevatorMotorA->Set(elevatorPower);
		elevatorMotorB->Set(elevatorPower);
		swingArmMotor->Set(-swingArmPower);

		// if integration of the ElevatorController class is desired call the following
		// function to periodically check the elevator height/angle and update the motor
		// controls accordingly
		// elevatorController->run();
//Camera and Servo
		camXServo->Set( (controlBox.GetX() * 0.5) + 0.5);

		if(b[4]){
			camYServo->Set(1.0);
		} else {
			camYServo->Set(0.0);
		}

// This handles the grappling solenoids
// button 3 is the momentary contact switch on the control box
// This same logic could be used on a joystick button if you wish
// The default should be with the grappling arms open and the code
// implements the default state as button 3 not pressed AND grappling = false

		if(b[3] and ! grappling) {
			dsGrappler->Set(DoubleSolenoid::kForward);
			grappling = true;
			Wait(.2);
			dsGrappler->Set(DoubleSolenoid::kOff);
		} else if (! b[3] and grappling)
		{
			dsGrappler->Set(DoubleSolenoid::kReverse); // Default initial state
			grappling = false;
			Wait(.2);
			dsGrappler->Set(DoubleSolenoid::kOff);
		}
	}

	void TestPeriodic() {
		lw->Run();
	}
};

START_ROBOT_CLASS(Robot);
