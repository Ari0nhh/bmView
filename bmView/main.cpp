#include "stdafx.h"
#include "bmview.h"
#include <QtWidgets/QApplication>
#include "intfs.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	concurrency::SchedulerPolicy sp(1, concurrency::MaxConcurrency, 10);
	concurrency::CurrentScheduler::Create(sp);

	IGlobalRendererPtr pRender = std::make_shared<bmView>();
	pRender->init();

	return a.exec();
}
