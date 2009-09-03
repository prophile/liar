%rv = type { i8*, i8* }
@cname.AssertionFailureException = private constant [26 x i8] c"AssertionFailureException\00"
@cname.RuntimeException = private constant [17 x i8] c"RuntimeException\00"

define %rv @exception_throw ( i8* %parentFrame, i8* %exception ) nounwind
{
entry:
	%entry.rv0 = insertvalue %rv undef, i8* null, 0
	%entry.rv1 = insertvalue %rv %entry.rv0, i8* %exception, 1
	ret %rv %entry.rv1
}

define internal fastcc %rv @exception_throw_class ( i8* %parentFrame, i64 %number ) nounwind
{
entry:
	%frame = call i8* @obj_allocate_stack_frame () nounwind
	%obj.exception = call i8* @obj_new ( i64 %number, i8* %frame ) nounwind
	%call.0 = call %rv @exception_throw ( i8* %frame, i8* %obj.exception ) nounwind
	%call.0.rv = extractvalue %rv %call.0, 0
	%call.0.exception = extractvalue %rv %call.0, 1
	%call.0.threw = icmp eq i8* %call.0.exception, null
	br i1 %call.0.threw, label %call.0.lpad, label %entry.cont.0
entry.cont.0:
	%entry.cont.0.rv0 = insertvalue %rv undef, i8* %call.0.rv, 0
	%entry.cont.0.rv1 = insertvalue %rv %entry.cont.0.rv0, i8* null, 1
	call void @obj_frame_transfer ( i8* %call.0.rv, i8* %frame, i8* %parentFrame ) nounwind
	call void @obj_release_stack_frame ( i8* %frame ) nounwind
	ret %rv %entry.cont.0.rv1
call.0.lpad:
	%call.0.lpad.rv0 = insertvalue %rv undef, i8* null, 0
	%call.0.lpad.rv1 = insertvalue %rv %call.0.lpad.rv0, i8* %call.0.exception, 1
	call void @obj_frame_transfer ( i8* %call.0.exception, i8* %frame, i8* %parentFrame ) nounwind
	call void @obj_release_stack_frame ( i8* %frame ) nounwind
	ret %rv %call.0.lpad.rv1
}

define %rv @exception_throw_runtime ( i8* %parentFrame ) nounwind
{
entry:
	%class = tail call i64 @class_lookup ( i8* getelementptr inbounds ([17 x i8]* @cname.RuntimeException, i32 0, i32 0) )
	%call = tail call fastcc %rv @exception_throw_class ( i8* %parentFrame, i64 %class ) nounwind
	ret %rv %call
}

define %rv @exception_assert ( i8* %parentFrame, i1 %condition )
{
entry:
	br i1 %condition, label %assert.ok, label %assert.failure
assert.ok:
	%assert.ok.rv0 = insertvalue %rv undef, i8* null, 0
	%assert.ok.rv1 = insertvalue %rv %assert.ok.rv0, i8* null, 1
	ret %rv %assert.ok.rv1
assert.failure:
	%class = tail call i64 @class_lookup ( i8* getelementptr inbounds ([26 x i8]* @cname.AssertionFailureException, i32 0, i32 0) )
	%call = tail call fastcc %rv @exception_throw_class ( i8* %parentFrame, i64 %class ) nounwind
	ret %rv %call
}

declare noalias i8* @obj_new ( i64 %classID, i8* %stackFrame ) nounwind
declare noalias i8* @obj_allocate_stack_frame () nounwind
declare %rv @obj_msg_send_arr ( i8* %stackFrame, i8* %obj, i64 %msgID, i64 %pcount, i8** %params ) nounwind
declare %rv @obj_msg_send ( i8* %stackFrame, i8* %obj, i64 %msgID, i64 %pcount, ... ) nounwind
declare %rv @class_msg_send_arr ( i8* %stackFrame, i64 %class, i64 %msgID, i64 %pcount, i8** %params ) nounwind
declare %rv @class_msg_send ( i8* %stackFrame, i64 %class, i64 %msgID, i64 %pcount, ... ) nounwind
declare void @obj_release_stack_frame ( i8* %frame ) nounwind
declare void @obj_write ( i8* %obj, i64 %index, i8* %target ) nounwind
declare void @obj_write_weak ( i8* %obj, i64 %index, i8* %target ) nounwind
declare i8* @obj_read ( i8* %obj, i64 %index ) nounwind
declare void @class_write_static ( i64 %class, i64 %index, i8* %target ) nounwind
declare void @class_write_static_weak ( i64 %class, i64 %index, i8* %target ) nounwind
declare i8* @class_read_static ( i64 %class, i64 %index ) nounwind
declare void @obj_frame_transfer ( i8* %object, i8* %frame, i8* %parentFrame ) nounwind
declare i64 @class_lookup ( i8* nocapture %name ) readnone nounwind
declare i64 @method_lookup ( i8* nocapture %name ) readnone nounwind
