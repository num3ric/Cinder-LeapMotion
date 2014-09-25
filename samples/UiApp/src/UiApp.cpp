/*
* 
* Copyright (c) 2014, Ban the Rewind
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or 
* without modification, are permitted provided that the following 
* conditions are met:
* 
* Redistributions of source code must retain the above copyright 
* notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright 
* notice, this list of conditions and the following disclaimer in 
* the documentation and/or other materials provided with the 
* distribution.
* 
* Neither the name of the Ban the Rewind nor the names of its 
* contributors may be used to endorse or promote products 
* derived from this software without specific prior written 
* permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*/

#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"
#include "Cinder-LeapMotion.h"

using namespace ci;

class UiApp : public app::AppNative
{
public:
	void						draw();
	void						prepareSettings( app::AppBasic::Settings* settings );
	void						resize();
	void						setup();
	void						update();
private:
	Leap::Frame					mFrame;
	LeapMotion::DeviceRef		mDevice;
	vec2						warpPointable( const Leap::Pointable& p );
	vec2						warpVector( const Leap::Vector& v );

	enum
	{
		GRAB, HAND, TOUCH, NONE
	} typedef CursorType;
	vec2						mCursorPosition;
	vec2						mCursorPositionTarget;
	CursorType					mCursorType;
	vec2						mFingerTipPosition;
	gl::TextureRef				mTexture[ 3 ];
	
	// UI
	gl::TextureRef				mButton[ 2 ];
	vec2						mButtonPosition[ 3 ];
	bool						mButtonState[ 3 ];
	gl::TextureRef				mSlider;
	vec2						mSliderPosition;
	gl::TextureRef				mTrack;
	vec2						mTrackPosition;
	
	// Params
	float						mFrameRate;
	bool						mFullScreen;
	params::InterfaceGlRef		mParams;

	// Save screen shot
	void						screenShot();
};

#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "Resources.h"

using namespace ci;
using namespace app;
using namespace LeapMotion;
using namespace std;

void UiApp::draw()
{
	gl::viewport( getWindowSize() );
	gl::clear( Colorf::white() );
	gl::setMatricesWindow( getWindowSize() );
	gl::color( ColorAf::white() );
	
	gl::enableAlphaBlending();
	gl::disableDepthRead();
	gl::disableDepthWrite();
	
	gl::pushMatrices();
	
	for ( size_t i = 0; i < 3; ++i ) {
		bool pressed = mButtonState[ i ];
		gl::pushMatrices();
		gl::translate( mButtonPosition[ i ] );
		gl::draw( mButton[ pressed ? 1 : 0 ] );
		gl::popMatrices();
	}
	
	// Draw slider
	gl::pushMatrices();
	gl::translate( mTrackPosition );
	gl::draw( mTrack );
	gl::popMatrices();
	gl::pushMatrices();
	gl::translate( mSliderPosition );
	gl::draw( mSlider );
	gl::popMatrices();
	
	// Draw cursor
	if ( mCursorType != CursorType::NONE ) {
		gl::color( ColorAf::white() );
		gl::pushMatrices();
		gl::translate( mCursorPosition );
		gl::draw( mTexture[ (size_t)mCursorType ] );
		gl::popMatrices();
	}
	
	gl::popMatrices();
	
	// Draw finger position for pressing buttons
	if ( mCursorType == CursorType::TOUCH ) {
		gl::color( ColorAf( 1.0f, 0.7f, 0.0f ) );
		gl::drawSolidCircle( mFingerTipPosition, 20.0f );
		gl::drawStrokedCircle( mFingerTipPosition, 40.0f );
	}
	
	mParams->draw();
}

void UiApp::prepareSettings( Settings* settings )
{
	settings->setWindowSize( 1024, 768 );
	settings->setFrameRate( 60.0f );
}

// Take screen shot
void UiApp::screenShot()
{
#if defined( CINDER_MSW )
	fs::path path = getAppPath();
#else
	fs::path path = getAppPath().parent_path();
#endif
	writeImage( path / fs::path( "frame" + toString( getElapsedFrames() ) + ".png" ), copyWindowSurface() );
}

void UiApp::resize()
{
	float h = (float)getWindowHeight() * 0.333f;
	float w = (float)getWindowWidth() * 0.25f;
	vec2 position( w, h );
	position -= vec2( mButton[ 0 ]->getSize() ) * 0.5f;
	for ( size_t i = 0; i < 3; ++i, position.x += w ) {
		mButtonPosition[ i ]	= position;
		mButtonState[ i ]		= false;
	}

	position = vec2( w * 2.0f, h * 2.0f );
	mTrackPosition		= position - vec2( mTrack->getSize() ) * 0.5f;
	mSliderPosition		= mTrackPosition;
	mSliderPosition.y	-= 45.0f;
}

void UiApp::setup()
{
	mDevice 		= Device::create();
	mDevice->connectEventHandler( [ & ]( Leap::Frame frame )
	{
		mFrame = frame;
	} );
	
	gl::Texture::Format format;
	format.setMagFilter( GL_NEAREST );
	format.setMinFilter( GL_NEAREST );
	for ( size_t i = 0; i < 3; ++i ) {
		switch ( (CursorType)i ) {
			case CursorType::GRAB:
				mTexture[ i ] = gl::Texture::create( loadImage( loadResource( RES_TEX_GRAB ) ), format );
				break;
			case CursorType::HAND:
				mTexture[ i ] = gl::Texture::create( loadImage( loadResource( RES_TEX_HAND ) ), format );
				break;
			case CursorType::TOUCH:
				mTexture[ i ] = gl::Texture::create( loadImage( loadResource( RES_TEX_TOUCH ) ), format );
				break;
			case NONE:
				break;
		}
	}
	
	mCursorType				= CursorType::NONE;
	mCursorPosition			= vec2( 0 );
	mCursorPositionTarget	= vec2( 0 );
	mFingerTipPosition		= vec2( 0 );
	
	mButton[ 0 ]	= gl::Texture::create( loadImage( loadResource( RES_TEX_BUTTON_OFF ) ), format );
	mButton[ 1 ]	= gl::Texture::create( loadImage( loadResource( RES_TEX_BUTTON_ON ) ), format );
	mSlider			= gl::Texture::create( loadImage( loadResource( RES_TEX_SLIDER ) ), format );
	mTrack			= gl::Texture::create( loadImage( loadResource( RES_TEX_TRACK ) ), format );

	
	mFrameRate	= 0.0f;
	mFullScreen	= false;
	mParams = params::InterfaceGl::create( "Params", ivec2( 200, 105 ) );
	mParams->addParam( "Frame rate",	&mFrameRate,				"", true );
	mParams->addParam( "Full screen",	&mFullScreen ).key( "f" );
	mParams->addButton( "Screen shot",	[ & ]() { screenShot(); },	"key=space" );
	mParams->addButton( "Quit",			[ & ]() { quit(); },		"key=q" );
	
	resize();
}

void UiApp::update()
{
	mFrameRate = getAverageFps();

	if ( mFullScreen != isFullScreen() ) {
		setFullScreen( mFullScreen );
	}

	// Interact with first hand only
	const Leap::HandList& hands = mFrame.hands();
	if ( hands.isEmpty() ) {
		mCursorType = CursorType::NONE;
	} else {
		const Leap::Hand& hand = *hands.begin();
		
		// Update cursor position
		mCursorPositionTarget	= warpVector( hand.stabilizedPalmPosition() );
		if ( mCursorType == CursorType::NONE ) {
			mCursorPosition = mCursorPositionTarget;
		}
		
		// Choose cursor type based on number of extended fingers
		switch ( hand.fingers().extended().count() ) {
			case 0:
				mCursorType	= CursorType::GRAB;
				
				// Slider
				if ( mCursorPosition.y > getWindowCenter().y ) {
					float x1			= mTrackPosition.x;
					float x2			= mTrackPosition.x + (float)( mTrack->getWidth() - mSlider->getWidth() );
					mSliderPosition.x	= math<float>::clamp( mCursorPosition.x, x1, x2 );
				}
				break;
			case 1:
				mCursorType	= CursorType::TOUCH;
				
				// Buttons
				mFingerTipPosition = warpPointable( *hand.fingers().begin() );
				for ( size_t i = 0; i < 3; ++i ) {
					mButtonState[ i ] = false;
					if ( mButton[ 0 ]->getBounds().contains( mFingerTipPosition - mButtonPosition[ i ] ) ) {
						mButtonState[ i ] = true;
					}
				}
				break;
			default:
				mCursorType	= CursorType::HAND;
				break;
		}
	}
	
	// Smooth cursor animation
	mCursorPosition = glm::mix( mCursorPosition, mCursorPositionTarget, 0.21f );
}

vec2 UiApp::warpPointable( const Leap::Pointable& p )
{
	vec3 result		= vec3( 0 );
	if ( mDevice ) {
		const Leap::Screen& screen = mDevice->getController()->locatedScreens().closestScreenHit( p );
		
		result		= LeapMotion::toVec3( screen.intersect( p, true, 1.0f ) );
	}
	result			*= vec3( vec2( getWindowSize() ), 0.0f );
	result.y		= (float)getWindowHeight() - result.y;
	return vec2( result );
}

vec2 UiApp::warpVector( const Leap::Vector& v )
{
	vec3 result		= vec3( 0 );
	if ( mDevice ) {
		const Leap::Screen& screen = mDevice->getController()->locatedScreens().closestScreen( v );
		
		result		= LeapMotion::toVec3( screen.project( v, true ) );
	}
	result			*= vec3( getWindowSize(), 0.0f );
	result.y		= (float)getWindowHeight() - result.y;
	return vec2( result );
}

CINDER_APP_NATIVE( UiApp, RendererGl( RendererGl::Options().antiAliasing( RendererGl::AA_NONE ) ) )
