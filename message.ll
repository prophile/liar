%rv = type { i8*, i8* }

define %rv @obj_msg_send_arr ( i8* %stackFrame, i8* %obj, i64 %msgID, i64 %pcount, i8** %params ) nounwind
{
entry:
	%isNull = icmp eq i8* %obj, null
	br i1 %isNull, label %wasnull, label %dispatch

dispatch:
	%method = call i8* @obj_lookup_method ( i8* %obj, i64 %msgID ) nounwind
	%method.cast = bitcast i8* %method to %rv (i8*, i8*, i64, i8**)*
	%result = tail call %rv %method.cast ( i8* %stackFrame, i8* %obj, i64 %pcount, i8** %params ) nounwind
	ret %rv %result

wasnull:
	%rv0 = insertvalue %rv undef, i8* null, 0
	%rv1 = insertvalue %rv %rv0, i8* null, 1
	ret %rv %rv1
}

define %rv @obj_msg_send ( i8* %stackFrame, i8* %obj, i64 %msgID, i64 %pcount, ... ) nounwind
{
entry:
	%ap = alloca i8*
	%ap2 = bitcast i8** %ap to i8*
	call void @llvm.va_start ( i8* %ap2 )
	%result = call %rv @obj_msg_send_arr ( i8* %stackFrame, i8* %obj, i64 %msgID, i64 %pcount, i8** %ap ) nounwind
	call void @llvm.va_end ( i8* %ap2 )
	ret %rv %result
}

define %rv @class_msg_send_arr ( i8* %stackFrame, i64 %class, i64 %msgID, i64 %pcount, i8** %params ) nounwind
{
entry:
	%method = call i8* @class_lookup_method ( i64 %class, i64 %msgID ) nounwind
	%method.cast = bitcast i8* %method to %rv (i8*, i64, i64, i8**)*
	%result = tail call %rv %method.cast ( i8* %stackFrame, i64 %class, i64 %pcount, i8** %params ) nounwind
	ret %rv %result
}

define %rv @class_msg_send ( i8* %stackFrame, i64 %class, i64 %msgID, i64 %pcount, ... ) nounwind
{
entry:
	%ap = alloca i8*
	%ap2 = bitcast i8** %ap to i8*
	call void @llvm.va_start ( i8* %ap2 )
	%result = call %rv @class_msg_send_arr ( i8* %stackFrame, i64 %class, i64 %msgID, i64 %pcount, i8** %ap ) nounwind
	call void @llvm.va_end ( i8* %ap2 )
	ret %rv %result
}

declare void @llvm.va_start(i8*)
declare void @llvm.va_end(i8*)
declare i8* @obj_lookup_method ( i8*, i64 ) nounwind
declare i8* @class_lookup_method ( i64, i64 ) nounwind
