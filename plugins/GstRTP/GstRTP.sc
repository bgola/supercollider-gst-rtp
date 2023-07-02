GstRTPOut : UGen {
	*ar { |id, input|
		/* TODO */
		^this.multiNew('audio', id, input);
	}
	checkInputs {
		/* TODO */
		^this.checkValidInputs;
	}
}
