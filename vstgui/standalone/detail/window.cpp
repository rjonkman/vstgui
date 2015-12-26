#include "window.h"
#include "../../lib/cframe.h"
#include "../../lib/dispatchlist.h"
#include "../icommand.h"
#include "../iwindowcontroller.h"
#include "platform/iplatformwindow.h"

//------------------------------------------------------------------------
namespace VSTGUI {
namespace Standalone {
namespace Detail {

//------------------------------------------------------------------------
class Window : public IWindow, public Platform::IWindowDelegate, public std::enable_shared_from_this<Window>
{
public:
	bool init (const WindowConfiguration& config, const WindowControllerPtr& controller);

	const WindowControllerPtr& getController () const override { return controller; }

	// IWindow
	CPoint getSize () const override { return platformWindow->getSize (); }
	CPoint getPosition () const override { return platformWindow->getPosition (); }
	void setSize (const CPoint& newSize) override { platformWindow->setSize (newSize); }
	void setPosition (const CPoint& newPosition) override { platformWindow->setPosition (newPosition); }
	void setTitle (const UTF8String& newTitle) override { platformWindow->setTitle (newTitle); }
	void setContentView (const SharedPointer<CFrame>& newFrame) override;
	void show () override { platformWindow->show (); }
	void hide () override { platformWindow->hide (); }
	void close () override { platformWindow->close (); }
	void registerWindowListener (IWindowListener* listener) override;
	void unregisterWindowListener (IWindowListener* listener) override;

	// Platform::IWindowDelegate
	CPoint constraintSize (const CPoint& newSize) override;
	void onSizeChanged (const CPoint& newSize) override;
	void onPositionChanged (const CPoint& newPosition) override;
	void onShow () override;
	void onHide () override;
	void onClosed () override;
	bool canClose () override;
	// ICommandHandler
	bool canHandleCommand (const Command& command) override;
	bool handleCommand (const Command& command) override;
private:
	WindowControllerPtr controller;
	Platform::WindowPtr platformWindow;
	SharedPointer<CFrame> frame;
	UTF8String autoSaveFrameName;
	DispatchList<IWindowListener> windowListeners;
};

//------------------------------------------------------------------------
bool Window::init (const WindowConfiguration& config, const WindowControllerPtr& inController)
{
	platformWindow = Platform::makeWindow (config, *this);
	if (platformWindow)
	{
		if (config.flags.doesAutoSaveFrame ())
			autoSaveFrameName = config.autoSaveFrameName;
		controller = inController;
	}
	return platformWindow != nullptr;
}

//------------------------------------------------------------------------
void Window::setContentView (const SharedPointer<CFrame>& newFrame)
{
	if (frame)
		frame->close ();
	frame = newFrame;
	if (!frame)
		return;
	frame->open (platformWindow->getPlatformHandle (), platformWindow->getPlatformType ());
}

//------------------------------------------------------------------------
CPoint Window::constraintSize (const CPoint& newSize)
{
	return controller ? controller->constraintSize (*this, newSize) : newSize;
}

//------------------------------------------------------------------------
void Window::onSizeChanged (const CPoint& newSize)
{
	windowListeners.forEach ([&] (IWindowListener* listener) {
		listener->onSizeChanged (*this, newSize);
	});
	if (controller)
		controller->onSizeChanged (*this, newSize);
	if (frame)
		frame->setSize (newSize.x, newSize.y);
}

//------------------------------------------------------------------------
void Window::onPositionChanged (const CPoint& newPosition)
{
	windowListeners.forEach ([&] (IWindowListener* listener) {
		listener->onPositionChanged (*this, newPosition);
	});
	if (controller)
		controller->onPositionChanged (*this, newPosition);
}

//------------------------------------------------------------------------
void Window::onClosed ()
{
	auto self = shared_from_this (); // make sure we live as long as this method executes
	windowListeners.forEach ([&] (IWindowListener* listener) {
		listener->onClosed (*this);
		windowListeners.remove (listener);
	});
	if (controller)
		controller->onClosed (*this);
	if (frame)
	{
		frame->remember ();
		frame->close ();
		frame = nullptr;
	}
	platformWindow = nullptr;
}

//------------------------------------------------------------------------
void Window::onShow ()
{
	windowListeners.forEach ([&] (IWindowListener* listener) {
		listener->onShow (*this);
	});
	if (controller)
		controller->onShow (*this);
}

//------------------------------------------------------------------------
void Window::onHide ()
{
	windowListeners.forEach ([&] (IWindowListener* listener) {
		listener->onHide (*this);
	});
	if (controller)
		controller->onHide (*this);
}

//------------------------------------------------------------------------
bool Window::canClose ()
{
	return controller ? controller->canClose (*this) : true;
}

//------------------------------------------------------------------------
void Window::registerWindowListener (IWindowListener* listener)
{
	windowListeners.add (listener);
}

//------------------------------------------------------------------------
void Window::unregisterWindowListener (IWindowListener* listener)
{
	windowListeners.remove (listener);
}

//------------------------------------------------------------------------
bool Window::canHandleCommand (const Command& command)
{
	if (auto commandHandler = controller->dynamicCast<ICommandHandler> ())
		return commandHandler->canHandleCommand (command);
	return false;
}

//------------------------------------------------------------------------
bool Window::handleCommand (const Command& command)
{
	if (auto commandHandler = controller->dynamicCast<ICommandHandler> ())
		return commandHandler->handleCommand (command);
	return false;
}

//------------------------------------------------------------------------
WindowPtr makeWindow (const WindowConfiguration& config, const WindowControllerPtr& controller)
{
	auto window = std::make_shared<Detail::Window> ();
	if (!window->init (config, controller))
		return nullptr;
	return window;
}

//------------------------------------------------------------------------
} // Detail
} // Standalone
} // VSTGUI
