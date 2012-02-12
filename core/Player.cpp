#include "Player.h"
#include <scx/Function.hpp>
#include <scx/FileHelp.hpp>
#include <scx/Conv.hpp>
#include <mous/IDecoder.h>
#include <mous/IRenderer.h>
using namespace std;
using namespace scx;
using namespace mous;

#include <iostream>

Player::Player():
    m_Status(MousStopped),
    m_StopDecoder(false),
    m_SuspendDecoder(true),
    m_pDecoder(NULL),
    m_SemWakeDecoder(0, 0),
    m_SemDecoderSuspended(0, 0),
    m_StopRenderer(false),
    m_SuspendRenderer(true),
    m_pRenderer(NULL),
    m_SemWakeRenderer(0, 0),
    m_SemRendererSuspended(0, 0),
    m_UnitBeg(0),
    m_UnitEnd(0),
    m_DecoderIndex(0),
    m_RendererIndex(0),
    m_UnitPerMs(0)
{
    m_FrameBuffer.AllocBuffer(5);

    m_ThreadForDecoder.Run(Function<void (void)>(&Player::WorkForDecoder, this));

    m_ThreadForRenderer.Run(Function<void (void)>(&Player::WorkForRenderer, this));
}

Player::~Player()
{
    m_StopDecoder = true;
    m_StopRenderer = true;
    m_SemWakeDecoder.Post();
    m_SemWakeRenderer.Post();
    //if (m_Status != MousStopped)
    //Stop();
    m_ThreadForDecoder.Join();
    m_ThreadForRenderer.Join();
}

PlayerStatus Player::GetStatus() const
{
    return m_Status;
}

void Player::AddDecoder(IDecoder* pDecoder)
{
    vector<string> list;
    pDecoder->GetFileSuffix(list);
    for (size_t i = 0; i < list.size(); ++i) {
	string suffix = ToLower(list[i]);
	DecoderMapIter iter = m_DecoderMap.find(suffix);
	if (iter == m_DecoderMap.end()) {
	    vector<IDecoder*>* dlist = new vector<IDecoder*>();
	    dlist->push_back(pDecoder);
	    m_DecoderMap.insert(DecoderMapPair(suffix, dlist));
	} else {
	    vector<IDecoder*>* dlist = iter->second;
	    dlist->push_back(pDecoder);
	}
    }
}

void Player::RemoveDecoder(IDecoder* pDecoder)
{
    /**
     * Stop in use.
     */

    /**
     * Remove it from map.
     */
    vector<string> list;
    pDecoder->GetFileSuffix(list);
    for (size_t i = 0; i < list.size(); ++i) {
	string suffix = ToLower(list[i]);
	DecoderMapIter iter = m_DecoderMap.find(suffix);
	if (iter != m_DecoderMap.end()) {
	    vector<IDecoder*>* dlist = iter->second;
	    for (size_t i = 0; i < dlist->size(); ++i) {
		if ((*dlist)[i] == pDecoder) {
		    dlist->erase(dlist->begin()+i);
		    break;
		}
	    }
	    if (dlist->empty()) {
		delete dlist;
		m_DecoderMap.erase(iter);
	    }
	}
    }
}

void Player::RemoveAllDecoders()
{
    /**
     * Stop in use.
     */

    /**
     * Clear all.
     */
    for(DecoderMapIter iter = m_DecoderMap.begin();
	iter != m_DecoderMap.end(); ++iter) {
	delete iter->second;
    }
    m_DecoderMap.clear();
}

void SpecifyDecoder(const string& suffix, IDecoder* pDecoder)
{

}

void Player::SetRenderer(IRenderer* pRenderer)
{
    m_pRenderer = pRenderer;
    pRenderer->OpenDevice("/dev/dsp");
}

void Player::UnsetRenderer()
{
    /**
     * Stop renderer.
     */
    m_pRenderer = NULL;
}

ErrorCode Player::Open(const string& path)
{
    string suffix = ToLower(FileSuffix(path));
    DecoderMapIter iter = m_DecoderMap.find(suffix);
    if (iter != m_DecoderMap.end()) {
	m_pDecoder = (*(iter->second))[0];
    } else {
	return MousPlayerNoDecoder;
    }

    if (m_pRenderer == NULL)
	return MousPlayerNoRenderer;

    m_UnitPerMs = (double)m_pDecoder->GetUnitCount() / m_pDecoder->GetDuration();

    uint32_t maxBytesPerUnit = m_pDecoder->GetMaxBytesPerUnit();
    for (size_t i = 0; i < m_FrameBuffer.GetBufferCount(); ++i) {
	FrameBuffer* buf = m_FrameBuffer.GetRawItem(i);
	if (buf->max < maxBytesPerUnit) {
	    if (buf->data != NULL)
		delete[] buf->data;
	    buf->data = new char[maxBytesPerUnit];
	    buf->used = 0;
	    buf->max = maxBytesPerUnit;
	}
    }

    ErrorCode err = m_pDecoder->Open(path);
    if (err != MousOk)
	return err;

    int32_t channels = m_pDecoder->GetChannels();
    int32_t sampleRate = m_pDecoder->GetSampleRate();
    int32_t bitsPerSample = m_pDecoder->GetBitRate();
    err = m_pRenderer->SetupDevice(channels, sampleRate, bitsPerSample);
    if (err != MousOk)
	return err;

    return MousOk;
}

void Player::Close()
{
    m_pDecoder->Close();
}

void Player::Play()
{
    uint64_t beg = 0;
    uint64_t end = m_pDecoder->GetUnitCount();
    PlayRange(beg, end);
}

void Player::Play(uint64_t msBegin, uint64_t msEnd)
{
    const uint64_t total = m_pDecoder->GetUnitCount();

    uint64_t beg = m_UnitPerMs * msBegin;
    if (beg > total)
	beg = total;

    uint64_t end = m_UnitPerMs * msEnd;
    if (end > total)
	end = total;

    PlayRange(beg, end);
}

void Player::PlayRange(uint64_t beg, uint64_t end)
{
    m_UnitBeg = beg;
    m_UnitEnd = end;

    m_DecoderIndex = m_UnitBeg;
    m_RendererIndex = m_UnitBeg;

    m_pDecoder->SetUnitIndex(m_UnitBeg);

    m_SuspendRenderer = false;
    m_SuspendDecoder = false;
    m_SemWakeDecoder.Post();
    m_SemWakeRenderer.Post();
}

void Player::Pause()
{
    cout << "Pause()" << endl;
    if (m_IsRendering) {
	m_SuspendRenderer = true;
	m_SemRendererSuspended.Wait();
    }

    cout << "Pause()1" << endl;
    if (m_IsDecoding) {
	m_SuspendDecoder = true;
	m_SemDecoderSuspended.Wait();
    }

    cout << "Pause() done" << endl;
}

void Player::Resume()
{
    m_SuspendRenderer = false;
    m_SuspendDecoder = false;
    m_SemWakeDecoder.Post();
    m_SemWakeRenderer.Post();
}

void Player::Stop()
{
    cout << "Stop()" << endl;
    if (m_IsRendering) {
	m_SuspendRenderer = true;
	m_FrameBuffer.RecycleFree(NULL);
	m_SemRendererSuspended.Wait();
    }

    cout << "Stop() 1" << endl;
    if (m_IsDecoding) {
	m_SuspendDecoder = true;
	m_FrameBuffer.RecycleData(NULL);
	m_SemDecoderSuspended.Wait();
    }

    m_FrameBuffer.ResetPV();

    cout << "Stop() done" << endl;
}

void Player::Seek(uint64_t msPos)
{
    uint64_t unitPos = m_UnitPerMs * msPos;
    if (unitPos > m_pDecoder->GetUnitCount())
	unitPos = m_pDecoder->GetUnitCount();
    m_pDecoder->SetUnitIndex(unitPos);
    m_DecoderIndex = unitPos;
    m_RendererIndex = unitPos;
}

uint64_t Player::GetDuration() const
{
    return m_pDecoder->GetDuration();
}

void Player::WorkForDecoder()
{
    while (true) {
	m_SemWakeDecoder.Wait();
	if (m_StopDecoder)
	    break;

	for (FrameBuffer* buf = NULL; ; ) {
	    buf = m_FrameBuffer.TakeFree();
	    m_pDecoder->ReadUnit(buf->data, buf->used);
	    m_FrameBuffer.RecycleFree(buf);
	    ++m_DecoderIndex;
	    if (m_DecoderIndex >= m_UnitEnd)
		break;

	    if (m_SuspendDecoder)
		break;
	}

	m_SemDecoderSuspended.Post();
    }
}

void Player::WorkForRenderer()
{
    while (true) {
	m_SemWakeRenderer.Wait();
	if (m_StopRenderer)
	    break;

	for (FrameBuffer* buf = NULL; ; ) {
	    cout << m_FrameBuffer.GetDataCount() << flush;
	    buf = m_FrameBuffer.TakeData();
	    m_pRenderer->WriteDevice(buf->data, buf->used);
	    m_FrameBuffer.RecycleData(buf);
	    ++m_RendererIndex;
	    if (m_RendererIndex >= m_UnitEnd)
		break;

	    if (m_SuspendRenderer)
		break;
	}

	m_SemRendererSuspended.Post();
    }
}
