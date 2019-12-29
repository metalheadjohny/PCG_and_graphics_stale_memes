#include "Controller.h"
#include "InputManager.h"
#include "CollisionEngine.h"
#include "Picker.h"


Controller::Controller() : _inMan(nullptr), dx(0), dy(0) {}

Controller::Controller(InputManager* inputManager) : _inMan(inputManager) {}

Controller::~Controller() {}



void Controller::processTransformationFPS(float dTime, SMatrix& transform)
{
	_inMan->getRelativeXY(dx, dy);	//bit lame that I solved it like this, coupled unnecessarily...
	
	if (!(dx == 0 && dy == 0))	//check if rotation happened, can skip a lot of work
	{
		SVec3 translation = transform.Translation();
		Math::SetTranslation(transform, SVec3());

		processRotationFPS(dTime, transform);

		Math::SetTranslation(transform, translation);
	}
	
	SVec3 velocityVector = processTranslationFPS(dTime, transform) * movCf * dTime;
	
	//flying mode is used for testing and shouldn't collide or fall for convenience
	if (!_isFlying)
	{
		resolveCollision(transform, dTime, velocityVector);
		applyGravity(dTime, transform);
	}

	Math::Translate(transform, velocityVector);
}



void Controller::processRotationFPS(float dTime, SMatrix& transformation) const
{
	SMatrix rh;
	rh = rh.CreateFromAxisAngle(SVec3::Up, DirectX::XMConvertToRadians(dx) * rotCf * dTime);

	transformation = transformation * rh;

	SVec3 right = transformation.Right();
	right.Normalize();

	SMatrix rv;
	rv = rv.CreateFromAxisAngle(right, DirectX::XMConvertToRadians(dy) * rotCf * dTime);

	transformation = transformation * rv;
}



SVec3 Controller::processTranslationFPS(const float dTime, const SMatrix& transformation) const
{
	SVec3 dir = -transformation.Forward();	//this is for rh... fml
	SVec3 right = SVec3::Up.Cross(dir);

	SVec3 deltaTranslation(0, 0, 0);

	if (_inMan->isKeyDown((short)'W')) {
		deltaTranslation = deltaTranslation + dir;
	}

	if (_inMan->isKeyDown((short)'S')) {
		deltaTranslation = deltaTranslation - dir;
	}

	if (_inMan->isKeyDown((short)'A')) {
		deltaTranslation = deltaTranslation - right;
	}

	if (_inMan->isKeyDown((short)'D')) {
		deltaTranslation = deltaTranslation + right;
	}

	if (_inMan->isKeyDown(VK_NUMPAD8)) {
		deltaTranslation = deltaTranslation + SVec3::Up;
	}

	if (_inMan->isKeyDown(VK_NUMPAD2)) {
		deltaTranslation = deltaTranslation - SVec3::Up;
	}

	deltaTranslation.Normalize();

	return deltaTranslation;
}



void Controller::processTransformationTP(float dTime, SMatrix& transform, SMatrix& camTransform)
{
	_inMan->getRelativeXY(dx, dy);

	processRotationTP(dTime, transform, camTransform);

	SVec3 velocityVector = processTranslationTP(dTime, transform, camTransform) * movCf * dTime;

	Math::Translate(transform, velocityVector);
	Math::Translate(camTransform, velocityVector);
}



SVec3 Controller::processTranslationTP(float dTime, const SMatrix & transformation, SMatrix& camTransform) const
{

	SVec3 dir = -transformation.Forward();	//this is for rh... fml
	SVec3 right = transformation.Right();

	SVec3 deltaTranslation(0, 0, 0);

	if (_inMan->isKeyDown((short)'W'))
	{
		deltaTranslation = deltaTranslation + dir;
	}

	if (_inMan->isKeyDown((short)'S'))
	{
		deltaTranslation = deltaTranslation - dir;
	}

	if (_inMan->isKeyDown((short)'Q'))
	{
		deltaTranslation = deltaTranslation - right;
	}

	if (_inMan->isKeyDown((short)'E'))
	{
		deltaTranslation = deltaTranslation + right;
	}

	deltaTranslation.Normalize();

	return deltaTranslation;
}



void Controller::processRotationTP(float dTime, SMatrix& transform, SMatrix& camTransform) const
{
	//old translation of the player
	SVec3 tempTranslation = transform.Translation();
	Math::SetTranslation(transform, SVec3());

	float rotator = 0;

	if (_inMan->isKeyDown((short)'A'))
		rotator += rotCf * .1f * dTime;

	if (_inMan->isKeyDown((short)'D'))
		rotator -= rotCf * .1f * dTime;

	transform *= SMatrix::CreateRotationY(-rotator);
	Math::SetTranslation(transform, tempTranslation);

	//camera rotation around player
	SVec3 playerToCam = Math::getNormalizedVec3(camTransform.Translation() - transform.Translation());

	SMatrix rh = SMatrix::CreateFromAxisAngle(SVec3::Up, DirectX::XMConvertToRadians(dx) * rotCf * dTime);
	Math::RotateVecByMat(playerToCam, rh);
	
	SMatrix rv = SMatrix::CreateFromAxisAngle(transform.Right(), DirectX::XMConvertToRadians(dy) * rotCf * dTime);
	Math::RotateVecByMat(playerToCam, rv);

	Math::SetTranslation(camTransform, transform.Translation() + playerToCam * camDist);

	camTransform = SMatrix::CreateWorld(camTransform.Translation(), playerToCam, SVec3(0, 1, 0));
}



void Controller::applyGravity(const float dTime, SMatrix& transformation) const	//get this thing out of here ffs
{
	if(!_grounded)
		Math::Translate(transformation, SVec3(0.f, -9.81f, 0.f) * dTime);
}



void Controller::toggleFlying()
{
	_isFlying = _isFlying ? false : true;
}



void Controller::setFlying(bool b)
{
	_isFlying = b;
}


//this shouldn't be here...
void Controller::resolveCollision(SMatrix& transformation, float dTime, SVec3& velocity)
{
	if (!_colEng)
		return;

	HitResult hr = _colEng->resolvePlayerCollision(transformation, velocity);

	if (hr.resolutionVector.LengthSquared() > .1f)
		_grounded = true;
	else
		_grounded = false;
}



void Controller::Observe(const KeyPressMessage& msg)
{
	switch (msg.key)
	{
	case 'F':
		if (msg.pressed) toggleFlying();	//every press down toggle, so I don't have to hold it
		break;

	case 'M':
		if (msg.pressed)
		{
			_showCursor = !_showCursor;
			ShowCursor(_showCursor);
		}
		break;

	default:
		break;
	}
}

void Controller::mouseLPressed(const MCoords& absmc)
{
	return;
}


void Controller::mouseLReleased(const MCoords& absmc)
{
	//pick from map

}


void Controller::mouseRPressed(const MCoords& absmc)
{
	return;
}


void Controller::mouseRReleased(const MCoords& absmc)
{
	return;
}