#include "RenderTarget.h"
#include <assert.h>

/////////////////////////////////////   CommonRenderTarget   ///////////////////////////////

CommonRenderTarget::CommonRenderTarget(ID3D11RenderTargetView* rtv) {
	m_renderTarget.Attach(rtv);
}

CommonRenderTarget::CommonRenderTarget() {}

////////////////////////////
// Public Function
////////////////////////////

ID3D11RenderTargetView* CommonRenderTarget::GetRTV() { 
	assert(m_renderTarget.Get() != NULL);
	return m_renderTarget.Get(); 
}
