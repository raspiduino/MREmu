#include "Audio.h"
#include <SFML/Audio.hpp>
#include <vmmm.h>

Midi::Midi(const char* file) {
	std::lock_guard lock(access_mutex);
	midi_player = adl_init(44100);
	if (!midi_player) {
		printf("Couldn't initialize ADLMIDI: %s\n", adl_errorString());
		error = true;
	}
	if (!error && adl_openFile(midi_player, file) < 0) {
		printf("Couldn't open music file: %s\n", adl_errorInfo(midi_player));
		error = true;
	}
	initialize(2, 44100);
}

Midi::Midi(void* buf, size_t len) {
	std::lock_guard lock(access_mutex);
	midi_player = adl_init(44100);
	if (!midi_player) {
		printf("Couldn't initialize ADLMIDI: %s\n", adl_errorString());
		error = true;
	}
	if (!error && adl_openData(midi_player, buf, len)) {
		printf("Couldn't open buffer: %s\n", adl_errorInfo(midi_player));
		error = true;
	}
	initialize(2, 44100);
}

bool Midi::onGetData(Chunk& data) {
	std::lock_guard lock(access_mutex);
	int samples_count = adl_play(midi_player, 4096, buffer);
	data.samples = buffer;
	data.sampleCount = samples_count;
	if(samples_count == 0)
		if (repeat) {
			repeat--;
			return true;
		}
		else
			return done = false;
	return true;
}

void Midi::onSeek(sf::Time timeOffset) {
	std::lock_guard lock(access_mutex);
	adl_positionSeek(midi_player, timeOffset.asSeconds());
}

Midi::~Midi() {
	std::lock_guard lock(access_mutex);
	adl_close(midi_player);
}

VMINT vm_midi_play_by_bytes(VMUINT8* midibuf, VMINT len, VMINT repeat, void (*f)(VMINT handle, VMINT event)) {
	return vm_midi_play_by_bytes_ex(midibuf, len, 0, repeat, 0, f);
}

VMINT vm_midi_play_by_bytes_ex(VMUINT8* midibuf, VMINT len, VMUINT start_time, 
	VMINT repeat, VMUINT path, void (*f)(VMINT handle, VMINT event)) 
{
	auto& audio = get_current_app_audio();

	std::shared_ptr<Midi> midi = std::make_shared<Midi>(midibuf, len);
	if (midi->error)
		return -1;

	midi->source = midibuf;
	midi->repeat = repeat;
	midi->setPlayingOffset(sf::milliseconds(start_time));
	midi->play();

	for (int i = 0; i < audio.midis.size(); ++i)
		if (audio.midis.is_active(i)
			&& audio.midis[i]->source == midibuf)
		{
			audio.midis[i]->stop();
			audio.midis[i].swap(midi);
			return i;
		}


	return audio.midis.push(midi);
}

VMINT vm_midi_pause(VMINT handle) {
	MREngine::AppAudio& audio = get_current_app_audio();

	if (audio.midis.is_active(handle)) {
		audio.midis[handle]->pause();
		return 0;
	}
	return -1;
}

VMINT vm_midi_get_time(VMINT handle, VMUINT* current_time) {
	MREngine::AppAudio& audio = get_current_app_audio();

	if (audio.midis.is_active(handle)) {
		*current_time = audio.midis[handle]->getPlayingOffset().
			asMilliseconds();;
		return 0;
	}
	return -1;
}

void vm_midi_stop(VMINT handle) {
	MREngine::AppAudio& audio = get_current_app_audio();

	if (audio.midis.is_active(handle)) {
		audio.midis[handle]->stop();
		audio.midis[handle].reset();
		audio.midis.remove(handle);
	}
}
void vm_midi_stop_all(void) {
	MREngine::AppAudio& audio = get_current_app_audio();

	audio.midis = ItemsMng<std::shared_ptr<Midi>>();
}
