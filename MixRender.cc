#include "MixRender.h"

#include <third_party/WebKit/WebKit/chromium/public/WebURL.h>
#include <third_party/WebKit/WebKit/chromium/public/WebURLRequest.h>
#include <third_party/WebKit/WebKit/chromium/public/WebRect.h>
#include <third_party/WebKit/WebKit/chromium/public/WebSettings.h>
#include <base/message_loop.h>
#include <webkit/glue/webkit_glue.h>
#include <skia/ext/bitmap_platform_device.h>

MixKit::MixRender::MixRender(int width, int height) :
    size(width, height),
    skiaCanvas(width, height, true),
    inMessageLoop(false),
    isLoadFinished(false)
{
    webView = WebKit::WebView::create(this, NULL);
    webView->initializeMainFrame(this);
    webView->resize(size);
    webView->mainFrame()->setCanHaveScrollbars(false);
    webView->settings()->setLoadsImagesAutomatically(true);
}

MixKit::MixRender::~MixRender() {
    webView->close();
}

bool MixKit::MixRender::loadURL(const std::string& url) {
    WebKit::WebFrame *webFrame = webView->mainFrame();
    webFrame->loadRequest(WebKit::WebURLRequest(WebKit::WebURL(GURL(url))));
    webView->layout();//XXX need this?
    while (!isLoadFinished) {
        inMessageLoop = true;
        MessageLoop::current()->Run();
        inMessageLoop = false;
    }
    
    //XXX Probably need to do this periodically when rendering video frames
    webFrame->collectGarbage();

    return true;
}

const SkBitmap& MixKit::MixRender::render() {
    webView->paint(webkit_glue::ToWebCanvas(&skiaCanvas), WebKit::WebRect(0, 0, size.width, size.height));

    // Get canvas bitmap
    skia::BitmapPlatformDevice &skiaDevice = static_cast<skia::BitmapPlatformDevice&>(skiaCanvas.getTopPlatformDevice());
    return skiaDevice.accessBitmap(false);
}

void MixKit::MixRender::didStopLoading() {
    isLoadFinished = true;
    if (inMessageLoop)
        MessageLoop::current()->Quit();
}

void MixKit::MixRender::didFailLoad(WebKit::WebFrame* frame, const WebKit::WebURLError& error) {
    //XXX need to handle - this means one of the frames failed - set flag so we can return from loadURL
    printf("Failed to load\n");
    isLoadFinished = true;
    if (inMessageLoop)
        MessageLoop::current()->Quit();
}
