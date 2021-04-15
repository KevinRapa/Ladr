function ret = ladr_check(data)
	global LADR;

	ret = true;

	if isempty(LADR) || LADR == 0
		return;
	end

	data_len = numel(data);
	if data_len == 0
		return;
	end

	% In case MATLAB reaches check first
	for tries = 1:5
		try
			desc = tcpclient('localhost', 10419);
			break;
		catch
			if tries == 5
				ret = false;
				return;
			end

			pause(1);
		end
	end

	% Get type of data to write from server
	typeid = recv_data(desc, 1, 'uint32');

	% 0 means stop
	if typeid == 0
		ret = false;
		delete(desc);
		return;
	elseif (~isreal(data) && (typeid == 1 || typeid == 2 || typeid == 3))
		% We have complex data, but server has real. Can't deal with this.
		write(desc, uint32(0)); % Tell server to stop
		ret = false;
		delete(desc);
		return;
	end

	% If data is a matrix, transpose it so we slice row-first
	% This doesn't modify the outside variable
	if size(data, 1) > 1
		data = data.';
	end

	% Send length of data and wait for echo back
	write(desc, uint32(data_len));
	echo = recv_data(desc, 1, 'uint32');

	if echo == 0 || echo ~= data_len
		ret = false;
		delete(desc);
		return;
	end

	% Get block size to use
	block_sz = double(recv_data(desc, 1, 'uint32'));
	if (block_sz == 0)
		ret = false;
		delete(desc);
		return;
	end

	chunks = floor(data_len / double(block_sz));
	remainder = mod(data_len, block_sz);

	index = uint32(1);

	for c = 1:chunks
		block = data(index : index + block_sz - 1);
		if ~send_data(desc, block, typeid)
			ret = false;
			delete(desc);
			return;
		end
		index = index + block_sz;
	end

	if remainder ~= 0
		block = data(index : index + remainder - 1);
		if ~send_data(desc, block, typeid)
			ret = false;
		end
	end

	delete(desc);
end

function ret = recv_data(desc, amnt, type)
	try
		ret = read(desc, amnt, type);
	catch
		ret = 0;
	end
end

function ret = send_data(desc, arr, typeid)
	ret = true;

	if ~isreal(arr)
		to_send = reshape([real(arr); imag(arr)], 1, []);
	elseif typeid == 4 || typeid == 5
		% If, for some reason, we have a real array but server expects complex
		to_send = reshape([arr; zeros(1, length(arr))], 1, []);
	else
		to_send = arr;
	end

	% Tell server length of block
	write(desc, uint32(length(arr)));

	switch typeid
	case 1
		write(desc, single(to_send))
	case 2
		write(desc, double(to_send))
	case 3
		write(desc, int32(to_send))
	case 4
		write(desc, single(to_send))
	case 5
		write(desc, double(to_send))
	otherwise
		ret = false;
		return;
	end

	% Wait for server to give permission to continue
	echo = recv_data(desc, 1, 'uint32');
	if echo == 0
		ret = false;
	end
end