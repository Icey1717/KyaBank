#include "renderer.h"

enum GS_PRIM
{
	GS_POINTLIST = 0,
	GS_LINELIST = 1,
	GS_LINESTRIP = 2,
	GS_TRIANGLELIST = 3,
	GS_TRIANGLESTRIP = 4,
	GS_TRIANGLEFAN = 5,
	GS_SPRITE = 6,
	GS_INVALID = 7,
};

void Renderer::CompatibilityMeshBuffer::Init(const int vertexCount, const int indexCount)
{
	vertices.reserve(vertexCount);
	indices.reserve(indexCount);

	vertex.buff = vertices.data();
	vertex.maxcount = vertexCount;

	index.buff = indices.data();
	index.maxcount = indexCount;
}

void Renderer::KickVertex(GSVertexUnprocessedNormal& vtx, GIFReg::GSPrim primReg, uint32_t skip, CompatibilityMeshBuffer& drawBuffer)
{
	GS_PRIM prim = (GS_PRIM)primReg.PRIM;
	assert(drawBuffer.vertex.tail < drawBuffer.vertex.maxcount + 3);

	size_t head = drawBuffer.vertex.head;
	size_t tail = drawBuffer.vertex.tail;
	size_t next = drawBuffer.vertex.next;
	size_t xy_tail = drawBuffer.vertex.xy_tail;

	drawBuffer.vertex.buff[drawBuffer.vertex.tail] = vtx;

	drawBuffer.vertex.tail = ++tail;
	drawBuffer.vertex.xy_tail = ++xy_tail;

	size_t n = 0;

	switch (prim)
	{
	case GS_POINTLIST: n = 1; break;
	case GS_LINELIST: n = 2; break;
	case GS_LINESTRIP: n = 2; break;
	case GS_TRIANGLELIST: n = 3; break;
	case GS_TRIANGLESTRIP: n = 3; break;
	case GS_TRIANGLEFAN: n = 3; break;
	case GS_SPRITE: n = 2; break;
	case GS_INVALID: n = 1; break;
	}

	size_t m = tail - head;

	if (m < n)
	{
		return;
	}

	if (skip != 0)
	{
		switch (prim)
		{
		case GS_POINTLIST:
		case GS_LINELIST:
		case GS_TRIANGLELIST:
		case GS_SPRITE:
		case GS_INVALID:
			drawBuffer.vertex.tail = head; // no need to check or grow the buffer length
			break;
		case GS_LINESTRIP:
		case GS_TRIANGLESTRIP:
			drawBuffer.vertex.head = head + 1;
			// fall through
		case GS_TRIANGLEFAN:
			assert(tail < drawBuffer.vertex.maxcount); // in case too many vertices were skipped
			break;
		default:
			__assume(0);
		}

		return;
	}

	assert(tail < drawBuffer.vertex.maxcount);
	assert(drawBuffer.index.tail < drawBuffer.index.maxcount);

	uint16_t* buff = &drawBuffer.index.buff[drawBuffer.index.tail];

	switch (prim)
	{
	case GS_POINTLIST:
		buff[0] = head + 0;
		drawBuffer.vertex.head = head + 1;
		drawBuffer.vertex.next = head + 1;
		drawBuffer.index.tail += 1;
		break;
	case GS_LINELIST:
		buff[0] = head + 0;
		buff[1] = head + 1;
		drawBuffer.vertex.head = head + 2;
		drawBuffer.vertex.next = head + 2;
		drawBuffer.index.tail += 2;
		break;
	case GS_LINESTRIP:
		if (next < head)
		{
			drawBuffer.vertex.buff[next + 0] = drawBuffer.vertex.buff[head + 0];
			drawBuffer.vertex.buff[next + 1] = drawBuffer.vertex.buff[head + 1];
			head = next;
			drawBuffer.vertex.tail = next + 2;
		}
		buff[0] = head + 0;
		buff[1] = head + 1;
		drawBuffer.vertex.head = head + 1;
		drawBuffer.vertex.next = head + 2;
		drawBuffer.index.tail += 2;
		break;
	case GS_TRIANGLELIST:
		buff[0] = head + 0;
		buff[1] = head + 1;
		buff[2] = head + 2;
		drawBuffer.vertex.head = head + 3;
		drawBuffer.vertex.next = head + 3;
		drawBuffer.index.tail += 3;
		break;
	case GS_TRIANGLESTRIP:
		if (next < head)
		{
			drawBuffer.vertex.buff[next + 0] = drawBuffer.vertex.buff[head + 0];
			drawBuffer.vertex.buff[next + 1] = drawBuffer.vertex.buff[head + 1];
			drawBuffer.vertex.buff[next + 2] = drawBuffer.vertex.buff[head + 2];
			head = next;
			drawBuffer.vertex.tail = next + 3;
		}
		buff[0] = head + 0;
		buff[1] = head + 1;
		buff[2] = head + 2;
		drawBuffer.vertex.head = head + 1;
		drawBuffer.vertex.next = head + 3;
		drawBuffer.index.tail += 3;
		break;
	case GS_TRIANGLEFAN:
		// TODO: remove gaps, next == head && head < tail - 3 || next > head && next < tail - 2 (very rare)
		buff[0] = head + 0;
		buff[1] = tail - 2;
		buff[2] = tail - 1;
		drawBuffer.vertex.next = tail;
		drawBuffer.index.tail += 3;
		break;
	case GS_SPRITE:
		buff[0] = head + 0;
		buff[1] = head + 1;
		drawBuffer.vertex.head = head + 2;
		drawBuffer.vertex.next = head + 2;
		drawBuffer.index.tail += 2;
		break;
	case GS_INVALID:
		drawBuffer.vertex.tail = head;
		break;
	default:
		__assume(0);
	}
}
